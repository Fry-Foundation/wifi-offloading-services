#include "lib/console.h"
#include "lib/http-requests.h"
#include "lib/scheduler.h"
#include "services/config.h"
#include "services/gen_id.h"
#include "services/mqtt.h"
#include "services/registration.h"
#include <curl/curl.h>
#include <json-c/json.h>
#include <mosquitto.h>
#include <services/access_token.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <time.h>

#define SPEEDTEST_ENDPOINT "monitoring/speedtest"
#define UPLOAD_LIMIT (config.speed_test_upload_limit * 1024 * 1024)

typedef struct {
    struct mosquitto *mosq;
    Registration *registration;
    AccessToken *access_token;
} SpeedTestTaskContext;

typedef struct {
    bool is_error;
    double upload_speed_mbps;
    double download_speed_mbps;
} SpeedTestResult;

typedef struct {
    bool is_error;
    double speed_mbps;
} TestResult;

static Console csl = {
    .topic = "speed test",
    .level = CONSOLE_DEBUG,
};

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t *total_bytes = (size_t *)userdata;
    *total_bytes += size * nmemb; // Count total bytes downloaded
    return size * nmemb;
}

TestResult measure_download_speed(char *access_token) {
    CURL *curl;
    CURLcode res;
    size_t total_bytes = 0;
    struct timeval start, end;
    char url[256];
    strcpy(url, config.speed_test_api);
    strcat(url, "/");
    strcat(url, "download");
    TestResult result = {
        .is_error = false,
        .speed_mbps = 0.0,
    };

    print_info(&csl, "Starting download speed test...");
    curl = curl_easy_init();
    if (!curl) {
        print_error(&csl, "Failed to initialize curl");
        result.is_error = true;
        curl_easy_cleanup(curl);
        return result;
    }
    print_debug(&csl, "Downloading file");
    struct curl_slist *headers = NULL;
    char auth_header[1024];
    snprintf(auth_header, sizeof(auth_header), "Access-Token: %s", access_token);
    headers = curl_slist_append(headers, auth_header);
    char bearer_header[1024];
    snprintf(bearer_header, sizeof(bearer_header), "Authorization: Bearer %s", config.speed_test_api_key);
    headers = curl_slist_append(headers, bearer_header);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &total_bytes);

    gettimeofday(&start, NULL);
    res = curl_easy_perform(curl);
    gettimeofday(&end, NULL);

    if (res != CURLE_OK) {
        print_error(&csl, "Error on download: %s", curl_easy_strerror(res));
        result.is_error = true;
    } else {
        double duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6; // Time in seconds
        double speed_mbps = (total_bytes * 8) / (duration * 1e6); // Calculate speed in Mbps
        double total_mb = total_bytes / (1024.0 * 1024.0); // Calculate total downloaded in MB
        print_info(&csl, "Download Speed: %.2f Mbps", speed_mbps);
        print_info(&csl, "Total Downloaded: %.2f MB", total_mb);
        result.speed_mbps = speed_mbps;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return result;
}

size_t upload_callback(void *ptr, size_t size, size_t nitems, void *userdata) {
    size_t *bytes_sent = (size_t *)userdata;
    size_t max_to_send = size * nitems;

    // If we've already sent the maximum amount, return 0 to signal end of upload
    if (*bytes_sent >= UPLOAD_LIMIT) {
        return 0;
    }

    size_t to_send = (*bytes_sent + max_to_send <= UPLOAD_LIMIT)
                         ? max_to_send
                         : UPLOAD_LIMIT - *bytes_sent;

    memset(ptr, 'A', to_send);
    *bytes_sent += to_send;

    return to_send;
}

TestResult measure_upload_speed(char *access_token) {
    CURL *curl;
    CURLcode res;
    size_t bytes_sent = 0;
    struct timeval start, end;
    char url[256];
    strcpy(url, config.speed_test_api);
    strcat(url, "/");
    strcat(url, "upload");
    TestResult result = {
        .is_error = false,
        .speed_mbps = 0.0,
    };

    print_info(&csl,"Starting upload speed test...");
    curl = curl_easy_init();
    if (!curl) {
        print_error(&csl,"Failed to initialize curl\n");
        result.is_error = true;
        curl_easy_cleanup(curl);
        return result;
    }

    struct curl_slist *headers = NULL;
    char auth_header[1024];
    snprintf(auth_header, sizeof(auth_header), "Access-Token: %s", access_token);
    headers = curl_slist_append(headers, auth_header);
    char bearer_header[1024];
    snprintf(bearer_header, sizeof(bearer_header), "Authorization: Bearer %s", config.speed_test_api_key);
    headers = curl_slist_append(headers, bearer_header);
    headers = curl_slist_append(headers, "Content-Type: application/octet-stream");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, upload_callback);
    curl_easy_setopt(curl, CURLOPT_READDATA, &bytes_sent);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)UPLOAD_LIMIT);

    gettimeofday(&start, NULL);
    res = curl_easy_perform(curl);
    gettimeofday(&end, NULL);

    if (res != CURLE_OK) {
        print_error(&csl,"Error on upload: %s", curl_easy_strerror(res));
        result.is_error = true;
    } else {
        double duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
        double speed_mbps = (bytes_sent * 8.0) / (duration * 1e6); // Speed in Mbps
        print_info(&csl,"Upload complete");
        print_info(&csl,"Upload Speed: %.2f Mbps", speed_mbps);
        print_info(&csl,"Total Uploaded: %.2f MB", bytes_sent / (1024.0 * 1024.0));
        result.speed_mbps = speed_mbps;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return result;
}

float get_average_latency(const char *hostname) {
    char command[256];
    snprintf(command, sizeof(command), "ping -c %d %s", config.speed_test_latency_attempts, hostname);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        print_error(&csl, "Failed to run ping command");
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

SpeedTestResult speed_test(char *bearer_token) {
    SpeedTestResult result = {
        .is_error = false,
        .upload_speed_mbps = 0.0,
        .download_speed_mbps = 0.0,
    };

    print_info(&csl, "Starting speed test");

    TestResult download_result = measure_download_speed(bearer_token);
    if (download_result.is_error) {
        print_error(&csl, "Download test failed");
        result.is_error = true;
        return result;
    }

    TestResult upload_result = measure_upload_speed(bearer_token);
    if (upload_result.is_error) {
        print_error(&csl, "Upload test failed");
        result.is_error = true;
        return result;
    }

    result.is_error = false;
    result.upload_speed_mbps = upload_result.speed_mbps;
    result.download_speed_mbps = download_result.speed_mbps;

    print_info(&csl, "Speed test complete");

    return result;
}

void speedtest_task(Scheduler *sch, void *task_context) {
    SpeedTestTaskContext *context = (SpeedTestTaskContext *)task_context;

    print_info(&csl, "Starting speedtest task");
    int interval = 0;
    int failed = 0;
    double upload_speed = 0.0;
    double download_speed = 0.0;
    float latency = get_average_latency("www.google.com");
    print_info(&csl, "Average latency: %.2f ms", latency);
    // while (interval < config.speed_test_backhaul_attempts) {
    //     SpeedTestResult result = speed_test(context->access_token->token);
    //     upload_speed += result.upload_speed_mbps;
    //     download_speed += result.download_speed_mbps;
    //     interval++;
    // }

    // double upload_speed_mbps = upload_speed / interval;
    // double download_speed_mbps = download_speed / interval;

    // print_info(&csl, "Average upload speed: %.2f Mbps", upload_speed_mbps);
    // print_info(&csl, "Average download speed: %.2f Mbps", download_speed_mbps);
    SpeedTestResult result = speed_test(context->access_token->token);

    time_t now;
    time(&now);
    char measurementid[256];
    generate_id(measurementid, sizeof(measurementid), context->registration->wayru_device_id, now);
    print_info(&csl, "Measurement ID for speedtest: %s", measurementid);
    json_object *speedtest_data = json_object_new_object();
    json_object_object_add(speedtest_data, "measurement_id", json_object_new_string(measurementid));
    json_object_object_add(speedtest_data, "device_id", json_object_new_string(context->registration->wayru_device_id));
    json_object_object_add(speedtest_data, "timestamp", json_object_new_int(now));
    json_object_object_add(speedtest_data, "upload_speed", json_object_new_double(result.upload_speed_mbps));
    json_object_object_add(speedtest_data, "download_speed", json_object_new_double(result.download_speed_mbps));
    json_object_object_add(speedtest_data, "latency", json_object_new_double(latency));
    const char *speedtest_data_str = json_object_to_json_string(speedtest_data);

    print_info(&csl, "Publishing speedtest results");
    publish_mqtt(context->mosq, "monitoring/speedtest", speedtest_data_str, 0);

    json_object_put(speedtest_data);

    // Schedule monitoring_task to rerun later
    unsigned int seed = time(0);
    const int random_speed_test_interval = rand_r(&seed) % (config.speed_test_maximum_interval - config.speed_test_minimum_interval + 1) + config.speed_test_minimum_interval;
    schedule_task(sch, time(NULL) + random_speed_test_interval, speedtest_task, "speedtest", context);
}

void speedtest_service(Scheduler *sch, struct mosquitto *mosq, Registration *registration, AccessToken *access_token) {
    if (config.speed_test_enabled == 0) {
        print_info(&csl, "Speedtest service is disabled by config");
        return;
    }

    SpeedTestTaskContext *context = (SpeedTestTaskContext *)malloc(sizeof(SpeedTestTaskContext));
    if (context == NULL) {
        print_error(&csl, "failed to allocate memory for speedtest task context");
        return;
    }

    context->mosq = mosq;
    context->registration = registration;
    context->access_token = access_token;

    unsigned int seed = time(0);
    const int random_speed_test_interval = rand_r(&seed) % (config.speed_test_maximum_interval - config.speed_test_minimum_interval + 1) + config.speed_test_minimum_interval;

    schedule_task(sch, time(NULL) + random_speed_test_interval, speedtest_task, "speedtest", context);
}
