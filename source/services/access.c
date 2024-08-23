#include "access.h"
#include "lib/console.h"
#include "lib/http-requests.h"
#include "lib/scheduler.h"
#include "services/config.h"
#include "services/device_info.h"
#include <json-c/json.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define KEY_FILE "access-key"
#define KEY_FILE_BUFFER_SIZE 768
#define REQUEST_BODY_BUFFER_SIZE 256
#define MAX_KEY_SIZE 1024
#define MAX_TIMESTAMP_SIZE 256
#define ACCESS_ENDPOINT "/api/nfnode/access-v2"
#define SCRIPTS_PATH "/etc/wayru-os-services/scripts"

typedef struct {
    DeviceInfo *device_info;
} AccessTaskContext;

AccessKey access_key = {.public_key = NULL, .issued_at_seconds = 0, .expires_at_seconds = 0};

time_t convert_to_time_t(char *timestamp_str) {
    long long int epoch = strtoll(timestamp_str, NULL, 10);
    return (time_t)epoch;
}

// @todo When opening fails, check if the file exists and create it
// @todo When memory allocation fails, try allocating memory at a different time or panic?
bool read_access_key() {
    console(CONSOLE_DEBUG, "reading stored access key");

    char access_file_path[KEY_FILE_BUFFER_SIZE];
    snprintf(access_file_path, sizeof(access_file_path), "%s/%s", config.data_path, KEY_FILE);

    FILE *file = fopen(access_file_path, "r");
    if (file == NULL) {
        console(CONSOLE_ERROR, "failed to open access key file");
        return false;
    }

    char line[MAX_KEY_SIZE];
    char issued_at_seconds[MAX_TIMESTAMP_SIZE];
    char expires_at_seconds[MAX_TIMESTAMP_SIZE];

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "public_key", 10) == 0) {
            if (access_key.public_key != NULL) {
                free(access_key.public_key);
            }

            // Subtract the length of "public_key" from the total length
            size_t key_length = strlen(line) - 11;
            access_key.public_key = malloc(key_length + 1);
            if (access_key.public_key == NULL) {
                console(CONSOLE_ERROR, "failed to allocate memory for key");
                fclose(file);
                return false;
            }

            strcpy(access_key.public_key, line + 11);
            access_key.public_key[key_length] = '\0';
        } else if (strncmp(line, "issued_at_seconds", 10) == 0) {
            sscanf(line, "issued_at_seconds %s", issued_at_seconds);
        } else if (strncmp(line, "expires_at_seconds", 10) == 0) {
            sscanf(line, "expires_at_seconds %s", expires_at_seconds);
        }
    }

    fclose(file);

    access_key.issued_at_seconds = convert_to_time_t(issued_at_seconds);
    access_key.expires_at_seconds = convert_to_time_t(expires_at_seconds);

    return true;
}

void write_access_key() {
    console(CONSOLE_DEBUG, "writing new access key");

    char access_file_path[KEY_FILE_BUFFER_SIZE];
    snprintf(access_file_path, sizeof(access_file_path), "%s/%s", config.data_path, KEY_FILE);

    FILE *file = fopen(access_file_path, "w");
    if (file == NULL) {
        console(CONSOLE_DEBUG, "Unable to open file for writing");
        return;
    }

    fprintf(file, "public_key %s\n", access_key.public_key);
    fprintf(file, "issued_at_seconds %ld\n", access_key.issued_at_seconds);
    fprintf(file, "expires_at_seconds %ld\n", access_key.expires_at_seconds);

    fclose(file);
}

bool check_access_key_near_expiration() {
    console(CONSOLE_DEBUG, "checking if key is near expiration");
    time_t now;
    time(&now);

    if (difftime(access_key.expires_at_seconds, now) <= 600) {
        console(CONSOLE_DEBUG, "key is near expiration");
        return true;
    } else {
        console(CONSOLE_DEBUG, "key is not near expiration");
        return false;
    }
}

bool request_access_key(DeviceInfo *device_info) {
    // Url
    char access_url[256];
    snprintf(access_url, sizeof(access_url), "%s%s", config.main_api, ACCESS_ENDPOINT);

    // Request body
    json_object *json_body = json_object_new_object();
    json_object_object_add(json_body, "device_id", json_object_new_string(device_info->device_id));
    json_object_object_add(json_body, "mac", json_object_new_string(device_info->mac));
    json_object_object_add(json_body, "name", json_object_new_string(device_info->name));
    json_object_object_add(json_body, "brand", json_object_new_string(device_info->brand));
    json_object_object_add(json_body, "model", json_object_new_string(device_info->model));
    json_object_object_add(json_body, "public_ip", json_object_new_string(device_info->public_ip));
    json_object_object_add(json_body, "os_name", json_object_new_string(device_info->os_name));
    json_object_object_add(json_body, "os_version", json_object_new_string(device_info->os_version));
    json_object_object_add(json_body, "os_services_version", json_object_new_string(device_info->os_services_version));
    json_object_object_add(json_body, "did_public_key", json_object_new_string(device_info->did_public_key));
    const char *body = json_object_to_json_string(json_body);

    console(CONSOLE_DEBUG, "access key request body %s", body);

    HttpPostOptions options = {
        .url = access_url,
        .body_json_str = body,
    };

    HttpResult result = http_post(&options);

    json_object_put(json_body);

    if (result.is_error) {
        console(CONSOLE_ERROR, "failed to request access key");
        console(CONSOLE_ERROR, "error: %s", result.error);
        return false;
    }

    if (result.response_buffer == NULL) {
        console(CONSOLE_ERROR, "failed to request access key");
        console(CONSOLE_ERROR, "no response received");
        return false;
    }

    // Parse response
    struct json_object *parsed_response;
    struct json_object *public_key;
    struct json_object *issued_at_seconds;
    struct json_object *expires_at_seconds;

    parsed_response = json_tokener_parse(result.response_buffer);
    if (parsed_response == NULL) {
        // JSON parsing failed
        console(CONSOLE_ERROR, "failed to parse access key JSON data");
        free(result.response_buffer);
        return false;
    }

    // Extract fields
    // Enhanced error logging
    bool error_occurred = false;
    if (!json_object_object_get_ex(parsed_response, "publicKey", &public_key)) {
        console(CONSOLE_ERROR, "publicKey field missing or invalid");
        error_occurred = true;
    }

    if (!json_object_object_get_ex(parsed_response, "issuedAtSeconds", &issued_at_seconds)) {
        console(CONSOLE_ERROR, "issuedAtSeconds field missing or invalid in payload");
        error_occurred = true;
    }

    if (!json_object_object_get_ex(parsed_response, "expiresAtSeconds", &expires_at_seconds)) {
        console(CONSOLE_ERROR, "expiresAtSeconds field missing or invalid in payload");
        error_occurred = true;
    }

    if (error_occurred) {
        json_object_put(parsed_response);
        free(result.response_buffer);
        return false;
    }

    if (access_key.public_key != NULL) {
        free(access_key.public_key);
    }

    access_key.public_key = malloc(strlen(json_object_get_string(public_key)) + 1); // +1 for null-terminator
    strcpy(access_key.public_key, json_object_get_string(public_key));
    access_key.issued_at_seconds = json_object_get_int64(issued_at_seconds);
    access_key.expires_at_seconds = json_object_get_int64(expires_at_seconds);

    json_object_put(parsed_response);
    free(result.response_buffer);

    return true;
};

void access_task(Scheduler *sch, void *task_context) {
    AccessTaskContext *context = (AccessTaskContext *)task_context;

    if (check_access_key_near_expiration()) {
        request_access_key(context->device_info);
        write_access_key();
    } else {
        console(CONSOLE_DEBUG, "key is still valid");
    }

    // Schedule the next key request
    schedule_task(sch, time(NULL) + config.access_interval, access_task, "access", context);
}

void access_service(Scheduler *sch, DeviceInfo *device_info) {
    AccessTaskContext *context = (AccessTaskContext *)malloc(sizeof(AccessTaskContext));
    if (context == NULL) {
        console(CONSOLE_ERROR, "failed to allocate memory for access token task context");
        return;
    }

    context->device_info = device_info;

    if (read_access_key()) {
        if (check_access_key_near_expiration()) {
            request_access_key(context->device_info);
            write_access_key();
        }
    } else {
        request_access_key(context->device_info);
        write_access_key();
    }

    console(CONSOLE_DEBUG, "access service");
    schedule_task(sch, time(NULL) + config.access_interval, access_task, "access", context);
}

void clean_access_service() {
    if (access_key.public_key != NULL) {
        free(access_key.public_key);
        access_key.public_key = NULL;
    }
}
