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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#define SPEEDTEST_ENDPOINT "monitoring/speedtest"
#define UPLOAD_LIMIT (config.speed_test_upload_limit * 1024 * 1024)
#define BUFFER_SIZE 1024
#define TEST_DURATION 10


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

TestResult measure_download_speed(int sockfd, struct sockaddr_in *server_addr, char *access_token) {
    char request[] = "DOWNLOAD";
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    long total_bytes = 0;
    struct timeval start, end;
    TestResult result = {
        .is_error = true,
        .speed_mbps = 0.0,
    };

    // Send request to server
    sendto(sockfd, request, strlen(request), 0, (struct sockaddr *)server_addr, sizeof(*server_addr));

    // Start timer
    print_info(&csl,"Starting download test...");

    gettimeofday(&start, NULL);
    //  Receive data from server
    while (1) {
        bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (bytes_received < 0) {
            print_error(&csl,"Error receiving data");
            break;
        }
        total_bytes += bytes_received;
        // Log time spent and total bytes received
        gettimeofday(&end, NULL);
        double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
        print_info(&csl, "Time spent: %.2f seconds, Total bytes received: %ld", elapsed_time, total_bytes);
        // Stop timer after TEST_DURATION seconds
        if (end.tv_sec - start.tv_sec >= TEST_DURATION) {
            break;
        }
    }

    // Calculate speed
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    double mbps = (total_bytes * 8) / (elapsed_time * 1e6);
    print_info(&csl,"Total MB Downloaded: %.2f MB", total_bytes / (1024.0 * 1024.0));
    result.is_error = false;
    result.speed_mbps = mbps;

    return result;
}

TestResult measure_upload_speed(int sockfd, struct sockaddr_in *server_addr, char *access_token) {
    char request[] = "UPLOAD";
    char buffer[BUFFER_SIZE];
    ssize_t bytes_sent;
    long total_bytes = 0;
    struct timespec start, end;
    TestResult result = {
        .is_error = false,
        .speed_mbps = 0.0,
    };

    // Send request to server
    sendto(sockfd, request, strlen(request), 0, (struct sockaddr *)server_addr, sizeof(*server_addr));
    print_info(&csl,"Starting upload test...");
    // Start timer
    gettimeofday(&start, NULL);

    //  Send data to server
    while (total_bytes < UPLOAD_LIMIT) {
        bytes_sent = sendto(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)server_addr, sizeof(*server_addr));
        if (bytes_sent < 0) {
            print_error(&csl,"Error sending data");
            break;
        }
        total_bytes += bytes_sent;

        // Stop timer after TEST_DURATION seconds
        gettimeofday(&end, NULL);
        if (end.tv_sec - start.tv_sec >= TEST_DURATION) {
            break;
        }
    }

    // Calculate speed
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double mbps = (total_bytes * 8) / (elapsed_time * 1e6);
    double total_mb = total_bytes / (1024.0 * 1024.0);
    print_info(&csl,"Total MB sent: %.2f MB", total_mb);
    result.speed_mbps = mbps;
    return result;
}

void test(char *access_token){

    int sockfd;
    struct sockaddr_in server_addr;
    
    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        print_error(&csl,"Error creating socket");
        return;
    }

    // Set server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        print_error(&csl,"Invalid address/ Address not supported");
        close(sockfd);
        return;
    }

    TestResult download_result = measure_download_speed(sockfd, &server_addr, access_token);
    if (download_result.is_error) {
        print_error(&csl, "Download speed test failed");
    } else {
        print_info(&csl, "Download speed: %.2f Mbps", download_result.speed_mbps);
    }

    TestResult upload_result = measure_upload_speed(sockfd, &server_addr, access_token);
    if (upload_result.is_error) {
        print_error(&csl, "Upload speed test failed");
    } else {
        print_info(&csl, "Upload speed: %.2f Mbps", upload_result.speed_mbps);
    }

    // Close the socket | ALWAYS MAKE SURE TO CLOSE SOCKET AFTER USE
    close(sockfd);
}

float get_average_latency(const char *hostname) {
    char command[256];
    snprintf(command, sizeof(command), "ping -c %d %s", config.speed_test_latency_attempts, hostname);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        console(CONSOLE_ERROR, "Failed to run ping command");
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

    console(CONSOLE_INFO, "Starting speed test");

    // TestResult download_result = measure_download_speed(bearer_token);
    // if (download_result.is_error) {
    //     print_error(&csl, "Download test failed");
    //     result.is_error = true;
    //     return result;
    // }

    // TestResult upload_result = measure_upload_speed(bearer_token);
    // if (upload_result.is_error) {
    //     print_error(&csl, "Upload test failed");
    //     result.is_error = true;
    //     return result;
    // }

    result.is_error = false;
    // result.upload_speed_mbps = upload_result.speed_mbps;
    // result.download_speed_mbps = download_result.speed_mbps;

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

    // console(CONSOLE_INFO, "Average upload speed: %.2f Mbps", upload_speed_mbps);
    // console(CONSOLE_INFO, "Average download speed: %.2f Mbps", download_speed_mbps);
    SpeedTestResult result = speed_test(context->access_token->token);

    time_t now;
    time(&now);
    char measurementid[256];
    generate_id(measurementid, sizeof(measurementid), context->registration->wayru_device_id, now);
    console(CONSOLE_INFO, "Measurement ID for speedtest: %s", measurementid);
    json_object *speedtest_data = json_object_new_object();
    json_object_object_add(speedtest_data, "measurement_id", json_object_new_string(measurementid));
    json_object_object_add(speedtest_data, "device_id", json_object_new_string(context->registration->wayru_device_id));
    json_object_object_add(speedtest_data, "timestamp", json_object_new_int(now));
    json_object_object_add(speedtest_data, "upload_speed", json_object_new_double(result.upload_speed_mbps));
    json_object_object_add(speedtest_data, "download_speed", json_object_new_double(result.download_speed_mbps));
    json_object_object_add(speedtest_data, "latency", json_object_new_double(latency));
    const char *speedtest_data_str = json_object_to_json_string(speedtest_data);

    console(CONSOLE_INFO, "Publishing speedtest results");
    publish_mqtt(context->mosq, "monitoring/speedtest", speedtest_data_str, 0);

    json_object_put(speedtest_data);

    // Schedule monitoring_task to rerun later
    unsigned int seed = time(0);
    const int random_speed_test_interval = rand_r(&seed) % (config.speed_test_maximum_interval - config.speed_test_minimum_interval + 1) + config.speed_test_minimum_interval;
    schedule_task(sch, time(NULL) + random_speed_test_interval, speedtest_task, "speedtest", context);
}

void speedtest_service(Scheduler *sch, struct mosquitto *mosq, Registration *registration, AccessToken *access_token) {
    if (config.speed_test_enabled == 0) {
        console(CONSOLE_INFO, "Speedtest service is disabled by config");
        return;
    }

    SpeedTestTaskContext *context = (SpeedTestTaskContext *)malloc(sizeof(SpeedTestTaskContext));
    if (context == NULL) {
        console(CONSOLE_ERROR, "failed to allocate memory for speedtest task context");
        return;
    }

    context->mosq = mosq;
    context->registration = registration;
    context->access_token = access_token;

    unsigned int seed = time(0);
    const int random_speed_test_interval = rand_r(&seed) % (config.speed_test_maximum_interval - config.speed_test_minimum_interval + 1) + config.speed_test_minimum_interval;

    schedule_task(sch, time(NULL) + random_speed_test_interval, speedtest_task, "speedtest", context);
}
