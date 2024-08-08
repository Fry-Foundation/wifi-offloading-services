#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <string.h>
#include "lib/console.h"
#include "lib/http-requests.h"
#include "env.h"
#include "access.h"

#define MEMORY_PERCENTAGE 0.5

typedef struct {
    double upload_speed_mbps, download_speed_mbps;
} SpeedTestResult;

char* get_available_memory_str() {
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

SpeedTestResult speed_test() {
    char *url = "http://192.168.56.1:4050/monitoring/speedtest";
    char *bearer_token = env("BEARER_TOKEN");
    console(CONSOLE_INFO, "Starting speed test\n");
    char *freeram_str = get_available_memory_str();
    char get_url[256];
    strcpy(get_url, url);
    strcat(get_url, "/");
    strcat(get_url, freeram_str);
    console(CONSOLE_DEBUG, "GET URL: %s\n", get_url);

    HttpResult download_result = download_test(get_url, bearer_token);
    if (download_result.is_error) {
        console(CONSOLE_ERROR, "Download test failed\n");
        return;
    }

    HttpResult upload_result = upload_test(url, bearer_token, download_result.response_buffer, download_result.response_size);
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

void speedtest_service(){
    console(CONSOLE_INFO, "Starting speedtest service\n");
    int interval = 0;
    double upload_speed = 0.0;
    double download_speed = 0.0;
    while (interval < 5) {
        SpeedTestResult result = speed_test();
        upload_speed += result.upload_speed_mbps;
        download_speed += result.download_speed_mbps;
        interval++;
    }
    SpeedTestResult result = {
        .upload_speed_mbps = upload_speed/interval,
        .download_speed_mbps = download_speed/interval,
    };

    console(CONSOLE_INFO, "Average upload speed: %.2f Mbps\n", result.upload_speed_mbps);
    console(CONSOLE_INFO, "Average download speed: %.2f Mbps\n", result.download_speed_mbps);
}