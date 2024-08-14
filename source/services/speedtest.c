#include "lib/console.h"
#include "lib/http-requests.h"
#include "lib/scheduler.h"
#include "services/config.h"
#include "services/mqtt.h"
#include "services/registration.h"
#include <curl/curl.h>
#include <json-c/json.h>
#include <mosquitto.h>
#include <services/access_token.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <time.h>

#define MEMORY_PERCENTAGE 0.5
#define NUM_PINGS 4
#define SPEEDTEST_ENDPOINT "monitoring/speedtest"

typedef struct {
    struct mosquitto *mosq;
    Registration *registration;
    AccessToken *access_token;
} SpeedTestTaskContext;

typedef struct {
    double upload_speed_mbps, download_speed_mbps;
} SpeedTestResult;

float get_average_latency(const char *hostname) {
    char command[256];
    snprintf(command, sizeof(command), "ping -c %d %s", NUM_PINGS, hostname);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        console(CONSOLE_ERROR, "Failed to run ping command\n");
        return -1;
    }

    char output[1024];
    float total_latency = 0.0;
    int executed_pings = 0;

    while (fgets(output, sizeof(output), fp) != NULL) {
        char *time_str = strstr(output, "time=");
        if (time_str != NULL) {
            float latency;
            if (sscanf(time_str, "time=%f", &latency) == 1) {
                total_latency += latency;
                executed_pings++;
            }
        }
    }

    pclose(fp);

    if (executed_pings > 0) {
        return total_latency / executed_pings;
    } else {
        return -1;
    }
}

char *get_available_memory_str() {
    struct sysinfo si;
    if (sysinfo(&si) != 0) {
        console(CONSOLE_ERROR, "sysinfo call failed");
        return NULL;
    }
    sysinfo(&si);
    size_t half_free_memory = (size_t)(si.freeram * MEMORY_PERCENTAGE);
    size_t buf_size = 20;
    char *mem_str = (char *)malloc(buf_size * sizeof(char));
    if (mem_str == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    snprintf(mem_str, buf_size, "%zu", half_free_memory);
    return mem_str;
}

HttpResult download_test(char *url, char *bearer_token) {
    HttpGetOptions options = {
        .url = url,
        .bearer_token = bearer_token,
    };

    struct timeval start, end;
    double time_taken = 0.0;

    gettimeofday(&start, NULL);
    HttpResult result = http_get(&options);
    gettimeofday(&end, NULL);
    time_taken = (end.tv_sec - start.tv_sec) * 1e6;
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;

    if (result.is_error) {
        console(CONSOLE_ERROR, "HTTP GET request failed: %s\n", result.error);
        free(result.error);
        free(result.response_buffer);
    } else {
        size_t total_bytes = result.response_size;
        double speed_bps = total_bytes / time_taken;
        double speed_mbps = (speed_bps * 8) / 1e6;
        console(CONSOLE_DEBUG, "Downloaded %zu bytes in %.2f seconds\n", total_bytes, time_taken);
        console(CONSOLE_DEBUG, "Download speed: %.2f Mbps\n", speed_mbps);
        result.download_speed_mbps = speed_mbps;
    }

    return result;
}

HttpResult upload_test(char *url, char *bearer_token, char *upload_data, size_t upload_data_size) {
    HttpPostOptions options = {
        .url = url,
        .bearer_token = bearer_token,
        .upload_data = upload_data,
        .upload_data_size = upload_data_size,
    };
    struct timeval start, end;
    double time_taken = 0.0;

    gettimeofday(&start, NULL);
    HttpResult result = http_post(&options);
    gettimeofday(&end, NULL);

    time_taken = (end.tv_sec - start.tv_sec) * 1e6;
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;

    if (result.is_error) {
        console(CONSOLE_ERROR, "HTTP POST request failed: %s\n", result.error);
        free(result.error);
        free(result.response_buffer);
    } else {
        double speed_bps = upload_data_size / time_taken;
        double speed_mbps = (speed_bps * 8) / 1e6;
        console(CONSOLE_DEBUG, "Uploaded %zu bytes in %.2f seconds\n", upload_data_size, time_taken);
        console(CONSOLE_DEBUG, "Upload speed: %.2f Mbps\n", speed_mbps);
        result.upload_speed_mbps = speed_mbps;
    }

    return result;
}

SpeedTestResult speed_test(char *bearer_token) {
    console(CONSOLE_INFO, "Starting speed test\n");

    char *freeram_str = get_available_memory_str();
    char get_url[256];
    strcpy(get_url, config.accounting_api);
    strcat(get_url, "/");
    strcat(get_url, SPEEDTEST_ENDPOINT);
    strcat(get_url, "/");
    strcat(get_url, freeram_str);
    console(CONSOLE_DEBUG, "GET URL: %s\n", get_url);

    HttpResult download_result = download_test(get_url, bearer_token);
    if (download_result.is_error) {
        console(CONSOLE_ERROR, "Download test failed\n");
        return;
    }

    char post_url[256];
    strcpy(post_url, config.accounting_api);
    strcat(post_url, "/");
    strcat(post_url, SPEEDTEST_ENDPOINT);
    console(CONSOLE_DEBUG, "POST URL: %s\n", post_url);

    HttpResult upload_result =
        upload_test(post_url, bearer_token, download_result.response_buffer, download_result.response_size);
    if (upload_result.is_error) {
        console(CONSOLE_ERROR, "Upload test failed\n");
        free(download_result.response_buffer);
        return;
    }

    SpeedTestResult result = {
        .upload_speed_mbps = upload_result.upload_speed_mbps,
        .download_speed_mbps = download_result.download_speed_mbps,
    };

    free(download_result.response_buffer);
    free(upload_result.response_buffer);

    console(CONSOLE_INFO, "Speed test complete\n");

    return result;
}

void speedtest_task(Scheduler *sch, void *task_context) {
    SpeedTestTaskContext *context = (SpeedTestTaskContext *)task_context;

    console(CONSOLE_INFO, "Starting speedtest task\n");
    int interval = 0;
    double upload_speed = 0.0;
    double download_speed = 0.0;
    float latency = get_average_latency("www.google.com");
    console(CONSOLE_INFO, "Average latency: %.2f ms\n", latency);
    while (interval < 5) {
        SpeedTestResult result = speed_test(context->access_token->token);
        upload_speed += result.upload_speed_mbps;
        download_speed += result.download_speed_mbps;
        interval++;
    }

    SpeedTestResult result = {
        .upload_speed_mbps = upload_speed / interval,
        .download_speed_mbps = download_speed / interval,
    };

    console(CONSOLE_INFO, "Average upload speed: %.2f Mbps\n", result.upload_speed_mbps);
    console(CONSOLE_INFO, "Average download speed: %.2f Mbps\n", result.download_speed_mbps);

    time_t now;
    time(&now);
    json_object *speedtest_data = json_object_new_object();
    json_object_object_add(speedtest_data, "device_id", json_object_new_string(context->registration->wayru_device_id));
    json_object_object_add(speedtest_data, "timestamp", json_object_new_int(now));
    json_object_object_add(speedtest_data, "upload_speed", json_object_new_double(result.upload_speed_mbps));
    json_object_object_add(speedtest_data, "download_speed", json_object_new_double(result.download_speed_mbps));
    json_object_object_add(speedtest_data, "latency", json_object_new_double(latency));
    const char *speedtest_data_str = json_object_to_json_string(speedtest_data);

    console(CONSOLE_INFO, "Publishing speedtest results\n");
    publish_mqtt(context->mosq, "monitoring/speedtest", speedtest_data_str);

    json_object_put(speedtest_data);

    schedule_task(sch, time(NULL) + config.device_status_interval, speedtest_task, "speedtest", context);
}

void speedtest_service(Scheduler *sch, struct mosquitto *mosq, Registration *registration, AccessToken *access_token) {
    SpeedTestTaskContext *context = (SpeedTestTaskContext *)malloc(sizeof(SpeedTestTaskContext));
    if (context == NULL) {
        console(CONSOLE_ERROR, "failed to allocate memory for speedtest task context");
        return;
    }

    context->mosq = mosq;
    context->registration = registration;
    context->access_token = access_token;

    speedtest_task(sch, context);
}
