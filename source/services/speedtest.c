#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <sys/time.h>
#include <string.h>
#include "lib/console.h"
#include "lib/http-requests.h"
#include "env.h"
#include "access.h"

HttpResult downloadTest(char *url, char *bearer_token) {
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
    } else {
        size_t total_bytes = strlen(result.response_buffer);
        console(CONSOLE_DEBUG, "Downloaded %zu bytes in %.2f seconds\n", total_bytes, time_taken);
        console(CONSOLE_DEBUG, "Download speed: %.2f bytes/second\n", total_bytes / time_taken);
    }

    return result;
}

HttpResult uploadTest(char *url, char *bearer_token, char *upload_data, size_t upload_data_size) {
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
    } else {
        size_t total_bytes = strlen(result.response_buffer);
        console(CONSOLE_DEBUG, "Uploaded %zu bytes in %.2f seconds\n", total_bytes, time_taken);
        console(CONSOLE_DEBUG, "Upload speed: %.2f bytes/second\n", total_bytes / time_taken);
    }

    return result;
}

void speedTest() {
    char *url = "http://192.168.56.1:4050/monitoring/speedtest";
    char *bearer_token = env("BEARER_TOKEN");
    console(CONSOLE_INFO, "Starting speed test\n");

    HttpResult download_result = downloadTest(url, bearer_token);
    if (download_result.is_error) {
        console(CONSOLE_ERROR, "Download test failed\n");
        return;
    }

    HttpResult upload_result = uploadTest(url, bearer_token, download_result.response_buffer, strlen(download_result.response_buffer));
    if (upload_result.is_error) {
        console(CONSOLE_ERROR, "Upload test failed\n");
        return;
    }

    free(download_result.response_buffer);
    free(upload_result.response_buffer);

    console(CONSOLE_INFO, "Speed test complete\n");
}