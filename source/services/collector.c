#include "collector.h"
#include "lib/console.h"
#include "lib/scheduler.h"
#include "lib/http-requests.h"
#include "lib/stats.h"
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

// Dynamic file size limits based on available disk space
static long max_log_file_size_bytes = 0;
static long emergency_truncate_size_bytes = 0;

// Memory failure tracking
static int consecutive_memory_failures = 0;
static const int MAX_MEMORY_FAILURES = 3;

// Calculate safe file size limits based on available disk space
static void calculate_file_size_limits(long available_disk_mb) {
    // Safety thresholds for different disk sizes
    if (available_disk_mb < 0) {
        // Fallback if disk space detection fails
        max_log_file_size_bytes = 1024 * 1024; // 1 MB
        emergency_truncate_size_bytes = 512 * 1024; // 512 KB
    } else if (available_disk_mb < 50) {
        // Very small storage (< 50 MB) - be very conservative
        max_log_file_size_bytes = 512 * 1024; // 512 KB (1% of 50MB)
        emergency_truncate_size_bytes = 256 * 1024; // 256 KB
    } else if (available_disk_mb < 500) {
        // Small storage (50 MB - 500 MB) - use 0.5%
        max_log_file_size_bytes = (available_disk_mb * 1024 * 1024) / 200; // 0.5%
        emergency_truncate_size_bytes = max_log_file_size_bytes / 2;
    } else if (available_disk_mb < 2048) {
        // Medium storage (500 MB - 2 GB) - use 0.2%
        max_log_file_size_bytes = (available_disk_mb * 1024 * 1024) / 500; // 0.2%
        emergency_truncate_size_bytes = max_log_file_size_bytes / 2;
    } else {
        // Large storage (> 2 GB) - cap at 5 MB maximum
        max_log_file_size_bytes = 5 * 1024 * 1024; // 5 MB
        emergency_truncate_size_bytes = 2 * 1024 * 1024; // 2.5 MB
    }
    
    // Ensure minimum viable log size (at least 100 KB)
    if (max_log_file_size_bytes < 100 * 1024) {
        max_log_file_size_bytes = 100 * 1024; // 100 KB minimum
        emergency_truncate_size_bytes = 50 * 1024; // 50 KB
    }
    
    printf("Collector: Set log limits based on %ld MB disk - Max: %ld KB, Truncate: %ld KB\n",
           available_disk_mb, 
           max_log_file_size_bytes / 1024,
           emergency_truncate_size_bytes / 1024);
}



// Console callback function that's compatible with ConsoleCallback signature
static void collector_console_callback(const char *topic, const char *level_label, const char *message) {
    // Use the collector_write function to handle structured logging
    collector_write(level_label, topic, message);
}

static long get_log_file_size(void) {
    if (!log_file) return 0;
    
    // Force flush any pending writes
    fflush(log_file);
    
    long current_pos = ftell(log_file);
    if (current_pos < 0) return 0;  // Error getting position
    
    if (fseek(log_file, 0, SEEK_END) != 0) return 0;  // Error seeking
    
    long size = ftell(log_file);
    if (size < 0) return 0;  // Error getting size
    
    if (fseek(log_file, current_pos, SEEK_SET) != 0) {
        // If we can't restore position, at least go to end
        fseek(log_file, 0, SEEK_END);
    }
    
    return size;
}

static bool emergency_rotate_log(void) {
    if (!log_file) return false;
    
    // Build log file path
    char log_file_path[256];
    snprintf(log_file_path, sizeof(log_file_path), "%s/%s", config.data_path, log_file_name);
    
    // Close current file handle
    fclose(log_file);
    log_file = NULL;
    
    // Read the file to keep the most recent entries
    FILE *read_file = fopen(log_file_path, "r");
    if (!read_file) {
        // If we can't read, just truncate everything
        log_file = fopen(log_file_path, "w");
        if (log_file) {
            fclose(log_file);
            log_file = fopen(log_file_path, "a");
        }
        return log_file != NULL;
    }
    
    // Get file size and calculate how much to keep
    fseek(read_file, 0, SEEK_END);
    long file_size = ftell(read_file);
    
    if (file_size <= emergency_truncate_size_bytes) {
        // File is already small enough, just reopen
        fclose(read_file);
        log_file = fopen(log_file_path, "a");
        return log_file != NULL;
    }
    
    // Read the last portion of the file (most recent logs)
    long start_pos = file_size - emergency_truncate_size_bytes;
    fseek(read_file, start_pos, SEEK_SET);
    
    // Find the start of a complete log line (after a newline)
    int c;
    while ((c = fgetc(read_file)) != EOF && c != '\n') {
        // Skip partial line
    }
    
    // Calculate remaining bytes to read
    long current_pos = ftell(read_file);
    long bytes_to_read = file_size - current_pos;
    if (bytes_to_read <= 0) {
        fclose(read_file);
        log_file = fopen(log_file_path, "w");
        if (log_file) {
            fclose(log_file);
            log_file = fopen(log_file_path, "a");
        }
        fprintf(stderr, "Emergency log rotation: truncated to 0 bytes\n");
        return log_file != NULL;
    }
    
    // Limit bytes to read to our buffer size
    if (bytes_to_read > emergency_truncate_size_bytes) {
        bytes_to_read = emergency_truncate_size_bytes - 1;
    }
    
    // Read the remaining content
    char *preserved_content = malloc(bytes_to_read + 1);
    if (!preserved_content) {
        fclose(read_file);
        log_file = fopen(log_file_path, "w");
        if (log_file) {
            fclose(log_file);
            log_file = fopen(log_file_path, "a");
        }
        return log_file != NULL;
    }
    
    size_t bytes_read = fread(preserved_content, 1, bytes_to_read, read_file);
    preserved_content[bytes_read] = '\0';
    fclose(read_file);
    
    // Rewrite the file with preserved content
    log_file = fopen(log_file_path, "w");
    if (log_file && bytes_read > 0) {
        fwrite(preserved_content, 1, bytes_read, log_file);
        fflush(log_file);
        fclose(log_file);
    } else if (log_file) {
        fclose(log_file);
    }
    
    free(preserved_content);
    
    // Reopen in append mode
    log_file = fopen(log_file_path, "a");
    
    fprintf(stderr, "Emergency log rotation: truncated to %zu bytes\n", bytes_read);
    return log_file != NULL;
}

// Helper function for emergency truncation to specific size
static bool emergency_truncate_to_size(const char *file_path, long target_size_bytes) {
    FILE *read_file = fopen(file_path, "r");
    if (!read_file) return false;
    
    // Get current file size
    fseek(read_file, 0, SEEK_END);
    long current_size = ftell(read_file);
    
    if (current_size <= target_size_bytes) {
        // File is already small enough
        fclose(read_file);
        return true;
    }
    
    // Keep the most recent portion of the file
    long start_pos = current_size - target_size_bytes;
    fseek(read_file, start_pos, SEEK_SET);
    
    // Find start of complete log line
    int c;
    while ((c = fgetc(read_file)) != EOF && c != '\n') {
        // Skip partial line
    }
    
    // Read the remaining content
    long remaining_size = current_size - ftell(read_file);
    if (remaining_size <= 0) {
        fclose(read_file);
        return false;
    }
    
    char *preserved_content = malloc(remaining_size + 1);
    if (!preserved_content) {
        fclose(read_file);
        return false;
    }
    
    size_t bytes_read = fread(preserved_content, 1, remaining_size, read_file);
    preserved_content[bytes_read] = '\0';
    fclose(read_file);
    
    // Rewrite file with preserved content
    FILE *write_file = fopen(file_path, "w");
    if (write_file && bytes_read > 0) {
        fwrite(preserved_content, 1, bytes_read, write_file);
        fclose(write_file);
    }
    
    free(preserved_content);
    return true;
}

bool collector_write(const char *level, const char *topic, const char *message) {
    if (!log_file) {
        return false;
    }

    // Don't check file size if limits haven't been initialized yet
    if (max_log_file_size_bytes == 0) {
        // Fallback to a reasonable default for early logs
        max_log_file_size_bytes = 1024 * 1024; // 1 MB
        emergency_truncate_size_bytes = 512 * 1024; // 512 KB
    }

    // Check file size before writing (with debug info)
    long current_size = get_log_file_size();
    if (current_size >= max_log_file_size_bytes) {
        fprintf(stderr, "Log file too large (%ld bytes >= %ld bytes), performing emergency rotation\n", 
                current_size, max_log_file_size_bytes);
        if (!emergency_rotate_log()) {
            fprintf(stderr, "Emergency rotation failed, skipping log entry\n");
            return false;
        }
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
    printf("Collector Backend: Starting send_logs_to_backend\n");
    printf("Collector Backend: Device ID: %s\n", device_id);
    printf("Collector Backend: API Host: %s\n", device_api_host);
    printf("Collector Backend: Log data length: %zu bytes\n", strlen(log_data));
    
    // Prepare JSON payload
    json_object *json_payload = json_object_new_object();
    json_object *json_device_id = json_object_new_string(device_id);
    json_object *json_logs = json_object_new_string(log_data);
    json_object *json_timestamp = json_object_new_int64(time(NULL));
    
    json_object_object_add(json_payload, "device_id", json_device_id);
    json_object_object_add(json_payload, "logs", json_logs);
    json_object_object_add(json_payload, "timestamp", json_timestamp);
    
    const char *json_string = json_object_to_json_string(json_payload);
    printf("Collector Backend: JSON payload size: %zu bytes\n", strlen(json_string));
    printf("Collector Backend: JSON payload preview (first 300 chars): %.300s%s\n", 
           json_string, strlen(json_string) > 300 ? "..." : "");
    
    // Build the full URL by concatenating host with endpoint
    char full_url[512];
    snprintf(full_url, sizeof(full_url), "%s%s", device_api_host, logs_endpoint);
    printf("Collector Backend: Full URL: %s\n", full_url);
    
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
    
    printf("Collector Backend: Calling http_post with bearer token: %s%.10s...\n", 
           access_token ? "" : "(null) ", access_token ? access_token : "");
    
    HttpResult result = http_post(&options);
    
    printf("Collector Backend: HTTP request completed\n");
    printf("Collector Backend: Is error: %s\n", result.is_error ? "true" : "false");
    if (result.is_error) {
        printf("Collector Backend: Error message: %s\n", result.error ? result.error : "(null)");
    }
    printf("Collector Backend: HTTP status code: %ld\n", result.http_status_code);
    if (result.response_buffer) {
        printf("Collector Backend: Response body: %s\n", result.response_buffer);
    } else {
        printf("Collector Backend: No response body\n");
    }
    
    // Clean up JSON object
    json_object_put(json_payload);
    
    // Check if request was successful
    if (result.is_error) {
        fprintf(stderr, "Collector Backend: Failed to send logs to backend: %s\n", result.error);
        if (result.response_buffer) {
            free(result.response_buffer);
        }
        return false;
    }
    
    // Check HTTP status code
    bool success = (result.http_status_code >= 200 && result.http_status_code < 300);
    if (!success) {
        fprintf(stderr, "Collector Backend: Backend returned HTTP status: %ld\n", result.http_status_code);
    } else {
        printf("Collector Backend: Request successful with HTTP status: %ld\n", result.http_status_code);
    }
    
    // Clean up response buffer
    if (result.response_buffer) {
        free(result.response_buffer);
    }
    
    printf("Collector Backend: Returning %s\n", success ? "true" : "false");
    return success;
}

void collector_task(Scheduler *sch, void *task_context) {
    CollectorContext *context = (CollectorContext *)task_context;
    
    printf("Collector: Starting collection task\n");

    // Build log file path using config.data_path
    char log_file_path[256];
    snprintf(log_file_path, sizeof(log_file_path), "%s/%s", config.data_path, log_file_name);
    
    printf("Collector: Looking for log file at: %s\n", log_file_path);

    // Read the log file
    FILE *read_file = fopen(log_file_path, "r");
    if (read_file) {
        printf("Collector: Successfully opened log file for reading\n");
        
        // Get file size
        fseek(read_file, 0, SEEK_END);
        long file_size = ftell(read_file);
        fseek(read_file, 0, SEEK_SET);
        
        printf("Collector: Log file size: %ld bytes\n", file_size);
        
        if (file_size > 0) {
            // Check available memory before attempting to read file
            unsigned long available_memory_kb = get_available_memory_kb();
            long file_size_kb = (file_size + 1023) / 1024; // Convert bytes to KB (rounded up)
            
            // Require at least 2x the file size in available memory for safety margin
            // This accounts for the file buffer + JSON processing + other operations
            long required_memory_kb = file_size_kb * 2;
            
            printf("Collector: Memory check - Available: %lu KB, Required: %ld KB, File: %ld KB\n",
                   available_memory_kb, required_memory_kb, file_size_kb);
            
            if (available_memory_kb == 0 || (long)available_memory_kb < required_memory_kb) {
                // Not enough memory available
                consecutive_memory_failures++;
                
                fprintf(stderr, "Collector: Insufficient memory to read log file (failure #%d). "
                               "File: %ld KB, Available: %lu KB, Required: %ld KB.\n",
                               consecutive_memory_failures, file_size_kb, available_memory_kb, required_memory_kb);
                
                // Take emergency action after consecutive failures
                if (consecutive_memory_failures >= MAX_MEMORY_FAILURES) {
                    fprintf(stderr, "Collector: Taking emergency action after %d consecutive memory failures\n", 
                           consecutive_memory_failures);
                    
                    // Emergency truncate the log file to fit in available memory
                    fclose(read_file);
                    read_file = NULL;
                    
                    // Calculate safe file size (use 1/4 of available memory, minimum 10KB)
                    long safe_size_bytes = ((long)available_memory_kb * 1024) / 4;
                    if (safe_size_bytes < 10240) safe_size_bytes = 10240; // 10KB minimum
                    
                    if (emergency_truncate_to_size(log_file_path, safe_size_bytes)) {
                        fprintf(stderr, "Collector: Emergency truncated log file to %ld bytes\n", safe_size_bytes);
                        
                        // Reduce future log file size limits to prevent recurrence
                        max_log_file_size_bytes = safe_size_bytes;
                        emergency_truncate_size_bytes = safe_size_bytes / 2;
                        
                        // Reset counter and try to read the truncated file
                        consecutive_memory_failures = 0;
                        
                        // Attempt to read the now-smaller file
                        read_file = fopen(log_file_path, "r");
                        if (read_file) {
                            fseek(read_file, 0, SEEK_END);
                            file_size = ftell(read_file);
                            fseek(read_file, 0, SEEK_SET);
                            
                            if (file_size > 0 && file_size <= safe_size_bytes) {
                                // Proceed with reading the truncated file
                                char *log_content = malloc(file_size + 1);
                                if (log_content) {
                                    size_t bytes_read = fread(log_content, 1, file_size, read_file);
                                    log_content[bytes_read] = '\0';
                                    fclose(read_file);
                                    read_file = NULL;
                                    
                                    if (send_logs_to_backend(context->device_id, context->access_token, 
                                                           log_content, context->device_api_host)) {
                                        // Truncate the file since we processed it successfully
                                        if (log_file) {
                                            fclose(log_file);
                                            log_file = fopen(log_file_path, "w");
                                            if (!log_file) {
                                                fprintf(stderr, "Failed to reopen log file for writing\n");
                                            }
                                        }
                                    }
                                    
                                    free(log_content);
                                } else {
                                    fclose(read_file);
                                    read_file = NULL;
                                }
                            } else {
                                if (read_file) {
                                    fclose(read_file);
                                    read_file = NULL;
                                }
                            }
                        }
                    }
                } else {
                    // Just skip this cycle and try again later
                    fclose(read_file);
                    read_file = NULL;
                }
            } else {
                // Sufficient memory available - reset failure counter
                consecutive_memory_failures = 0;
                printf("Collector: Memory check passed, proceeding to read file\n");
                
                // Sufficient memory available - proceed with reading file
                char *log_content = malloc(file_size + 1);
                if (log_content) {
                    printf("Collector: Successfully allocated %ld bytes for log content\n", file_size + 1);
                    
                    size_t bytes_read = fread(log_content, 1, file_size, read_file);
                    log_content[bytes_read] = '\0';
                    
                    printf("Collector: Read %zu bytes from log file\n", bytes_read);
                    
                    // Close read file before sending to backend
                    fclose(read_file);
                    read_file = NULL;
                    
                    printf("Collector: Attempting to send logs to backend\n");
                    printf("Collector: Device ID: %s\n", context->device_id);
                    printf("Collector: API Host: %s\n", context->device_api_host);
                    printf("Collector: Log content preview (first 200 chars): %.200s%s\n", 
                           log_content, strlen(log_content) > 200 ? "..." : "");
                    
                    // Send the log file to the backend
                    if (send_logs_to_backend(context->device_id, context->access_token, log_content, context->device_api_host)) {
                        printf("Collector: Successfully sent logs to backend\n");
                        // Successfully sent logs, truncate the file
                        if (log_file) {
                            fclose(log_file);
                            log_file = fopen(log_file_path, "w"); // Truncate file
                            if (!log_file) {
                                fprintf(stderr, "Collector: Failed to reopen log file for writing\n");
                            } else {
                                printf("Collector: Successfully truncated log file after sending\n");
                            }
                        }
                    } else {
                        fprintf(stderr, "Collector: Failed to send logs to backend\n");
                    }
                    
                    free(log_content);
                    log_content = NULL;
                } else {
                    // malloc failed - this shouldn't happen since we checked memory above
                    fprintf(stderr, "Collector: Failed to allocate memory for log file despite availability check\n");
                    fclose(read_file);
                    read_file = NULL;
                }
            }
        } else {
            printf("Collector: Log file is empty (0 bytes), nothing to send\n");
            fclose(read_file);
            read_file = NULL;
        }
    } else {
        printf("Collector: Failed to open log file for reading: %s\n", log_file_path);
    }
    
    // Reschedule the task with the scheduler from lib/scheduler.c
    printf("Collector: Rescheduling next collection in %d seconds\n", context->collector_interval);
    schedule_task(sch, time(NULL) + context->collector_interval, collector_task, "collector", task_context);
    printf("Collector: Collection task completed\n");
}

void collector_service(Scheduler *sch, char *device_id, char *access_token, int collector_interval, const char *device_api_host) {
    // Get available disk space and calculate dynamic file size limits
    long available_disk_mb = get_available_disk_space_mb(config.data_path);
    calculate_file_size_limits(available_disk_mb);
    
    // Debug: Show the calculated limits
    printf("Collector debug: max_log_file_size_bytes = %ld, emergency_truncate_size_bytes = %ld\n", 
           max_log_file_size_bytes, emergency_truncate_size_bytes);

    // Allocate context once - it will persist across all scheduled task executions
    CollectorContext *context = malloc(sizeof(CollectorContext));
    if (!context) {
        fprintf(stderr, "Failed to allocate memory for collector context\n");
        return;
    }
    
    // Copy strings to ensure they remain valid throughout service lifetime
    context->device_id = strdup(device_id);
    context->access_token = strdup(access_token);
    context->collector_interval = collector_interval;
    context->device_api_host = strdup(device_api_host);
    
    if (!context->device_id || !context->access_token || !context->device_api_host) {
        fprintf(stderr, "Failed to allocate memory for collector context strings\n");
        // Cleanup partial allocation
        if (context->device_id) free(context->device_id);
        if (context->access_token) free(context->access_token);
        if (context->device_api_host) free(context->device_api_host);
        free(context);
        return;
    }

    // Start the collector task - context will be reused across all executions
    collector_task(sch, context);
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
