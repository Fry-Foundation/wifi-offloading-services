#include "collector.h"
#include "lib/console.h"
#include "lib/scheduler.h"
#include "lib/http-requests.h"
#include "services/config/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <json-c/json.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    char *device_id;
    char *access_token;
    int collector_interval;
    char *device_api_host;
} CollectorContext;

// File handle for structured logging
static FILE *log_file = NULL;
static const char *logs_endpoint = "/logs";
static const char *log_file_name = "collector.log";

// Console callback function that's compatible with ConsoleCallback signature
static void collector_console_callback(const char *topic, const char *level_label, const char *message) {
    // Use the collector_write function to handle structured logging
    collector_write(level_label, topic, message);
}

bool collector_write(const char *level, const char *topic, const char *message) {
    if (!log_file) {
        return false;
    }

    // Get current timestamp
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[64];
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    // Create JSON object using json-c
    json_object *json_log = json_object_new_object();
    json_object *json_timestamp = json_object_new_string(timestamp);
    json_object *json_level = json_object_new_string(level);
    json_object *json_topic = json_object_new_string(topic);
    json_object *json_message = json_object_new_string(message);
    
    // Add fields to JSON object
    json_object_object_add(json_log, "timestamp", json_timestamp);
    json_object_object_add(json_log, "level", json_level);
    json_object_object_add(json_log, "topic", json_topic);
    json_object_object_add(json_log, "message", json_message);
    
    // Write JSON string to file
    const char *json_string = json_object_to_json_string(json_log);
    int result = fprintf(log_file, "%s\n", json_string);
    
    // Clean up JSON object
    json_object_put(json_log);
    
    if (result > 0) {
        fflush(log_file); // Ensure immediate write
        return true;
    }
    
    return false;
}

void collector_init(void) {
    // Make sure the data directory exists (following config.data_path)
    struct stat st = {0};
    if (stat(config.data_path, &st) == -1) {
        if (mkdir(config.data_path, 0755) != 0) {
            fprintf(stderr, "Failed to create data directory: %s\n", config.data_path);
            return;
        }
    }

    // Build log file path using config.data_path
    char log_file_path[256];
    snprintf(log_file_path, sizeof(log_file_path), "%s/%s", config.data_path, log_file_name);

    // Create/open the log file
    log_file = fopen(log_file_path, "a");
    if (!log_file) {
        fprintf(stderr, "Failed to open log file: %s\n", log_file_path);
        return;
    }

    // Register the callback with the console system
    console_set_callback(collector_console_callback);
    
    printf("Collector service initialized - logging to %s\n", log_file_path);
}

static bool send_logs_to_backend(const char *device_id, const char *access_token, const char *log_data, const char *device_api_host) {
    // Prepare JSON payload
    json_object *json_payload = json_object_new_object();
    json_object *json_device_id = json_object_new_string(device_id);
    json_object *json_logs = json_object_new_string(log_data);
    json_object *json_timestamp = json_object_new_int64(time(NULL));
    
    json_object_object_add(json_payload, "device_id", json_device_id);
    json_object_object_add(json_payload, "logs", json_logs);
    json_object_object_add(json_payload, "timestamp", json_timestamp);
    
    const char *json_string = json_object_to_json_string(json_payload);
    
    // Build the full URL by concatenating host with endpoint
    char full_url[512];
    snprintf(full_url, sizeof(full_url), "%s%s", device_api_host, logs_endpoint);
    
    // Use the HTTP abstraction library
    HttpPostOptions options = {
        .url = full_url,
        .bearer_token = access_token,
        .body_json_str = json_string,
        .legacy_key = NULL,
        .upload_file_path = NULL,
        .upload_data = NULL,
        .upload_data_size = 0
    };
    
    HttpResult result = http_post(&options);
    
    // Clean up JSON object
    json_object_put(json_payload);
    
    // Check if request was successful
    if (result.is_error) {
        fprintf(stderr, "Failed to send logs to backend: %s\n", result.error);
        if (result.response_buffer) {
            free(result.response_buffer);
        }
        return false;
    }
    
    // Check HTTP status code
    bool success = (result.http_status_code >= 200 && result.http_status_code < 300);
    if (!success) {
        fprintf(stderr, "Backend returned HTTP status: %ld\n", result.http_status_code);
    }
    
    // Clean up response buffer
    if (result.response_buffer) {
        free(result.response_buffer);
    }
    
    return success;
}

void collector_task(Scheduler *sch, void *task_context) {
    CollectorContext *context = (CollectorContext *)task_context;

    // Build log file path using config.data_path
    char log_file_path[256];
    snprintf(log_file_path, sizeof(log_file_path), "%s/%s", config.data_path, log_file_name);

    // Read the log file
    FILE *read_file = fopen(log_file_path, "r");
    if (read_file) {
        // Get file size
        fseek(read_file, 0, SEEK_END);
        long file_size = ftell(read_file);
        fseek(read_file, 0, SEEK_SET);
        
        if (file_size > 0) {
            // Allocate buffer and read file content
            char *log_content = malloc(file_size + 1);
            if (log_content) {
                size_t bytes_read = fread(log_content, 1, file_size, read_file);
                log_content[bytes_read] = '\0';
                
                // Send the log file to the backend
                if (send_logs_to_backend(context->device_id, context->access_token, log_content, context->device_api_host)) {
                    // Successfully sent logs, truncate the file
                    fclose(read_file);
                    if (log_file) {
                        fclose(log_file);
                        log_file = fopen(log_file_path, "w"); // Truncate file
                        if (!log_file) {
                            fprintf(stderr, "Failed to reopen log file for writing\n");
                        }
                    }
                } else {
                    fprintf(stderr, "Failed to send logs to backend\n");
                }
                
                free(log_content);
            }
        }
        
        if (read_file != log_file) {
            fclose(read_file);
        }
    }
    
    // Reschedule the task with the scheduler from lib/scheduler.c
    schedule_task(sch, time(NULL) + context->collector_interval, collector_task, "collector", task_context);
}

void collector_service(Scheduler *sch, char *device_id, char *access_token, int collector_interval, const char *device_api_host) {
    CollectorContext context = {
        .device_id = device_id,
        .access_token = access_token,
        .collector_interval = collector_interval,
        .device_api_host = (char *)device_api_host
    };

    collector_task(sch, &context);
}

void collector_cleanup(void) {
    // Unregister callback
    console_set_callback(NULL);
    
    // Close log file
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    
    printf("Collector service cleaned up\n");
}
