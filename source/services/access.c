#include "access.h"
#include "lib/console.h"
#include "lib/curl_helpers.h"
#include "lib/scheduler.h"
#include "lib/script_runner.h"
#include "services/config.h"
#include "services/device_data.h"
#include "services/setup.h"
#include <curl/curl.h>
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
        console(CONSOLE_ERROR, "failed to open key file");
        return false;
    }

    char line[MAX_KEY_SIZE];
    char issued_at_seconds[MAX_TIMESTAMP_SIZE];
    char expires_at_seconds[MAX_TIMESTAMP_SIZE];

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "public_key", 10) == 0) {
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

bool request_access_key() {
    CURL *curl;
    CURLcode res;

    char *response_buffer = init_response_buffer();

    curl = curl_easy_init();

    if (!curl) {
        console(CONSOLE_ERROR, "access key curl could not be initialized");
        free(response_buffer);
        return false;
    }

    // Set up as POST request
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    // Url
    char access_url[256];
    snprintf(access_url, sizeof(access_url), "%s%s", config.main_api, ACCESS_ENDPOINT);
    curl_easy_setopt(curl, CURLOPT_URL, access_url);

    // Request headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Request body
    json_object *json_body = json_object_new_object();
    json_object_object_add(json_body, "device_id", json_object_new_string(device_data.device_id));
    json_object_object_add(json_body, "mac", json_object_new_string(device_data.mac));
    json_object_object_add(json_body, "name", json_object_new_string(device_data.name));
    json_object_object_add(json_body, "brand", json_object_new_string(device_data.brand));
    json_object_object_add(json_body, "model", json_object_new_string(device_data.model));
    json_object_object_add(json_body, "public_ip", json_object_new_string(device_data.public_ip));
    json_object_object_add(json_body, "os_name", json_object_new_string(device_data.os_name));
    json_object_object_add(json_body, "os_version", json_object_new_string(device_data.os_version));
    json_object_object_add(json_body, "os_services_version", json_object_new_string(device_data.os_services_version));
    json_object_object_add(json_body, "did_public_key", json_object_new_string(device_data.did_public_key));
    const char *body = json_object_to_json_string(json_body);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);

    console(CONSOLE_DEBUG, "access key request body %s", body);

    // Response callback and buffer
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, save_to_buffer_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);

    res = curl_easy_perform(curl);

    json_object_put(json_body);

    if (res != CURLE_OK) {
        console(CONSOLE_ERROR, "access key curl request failed: %s", curl_easy_strerror(res));
        free(response_buffer);
        return false;
    }

    // Parse response
    struct json_object *parsed_response;
    struct json_object *public_key;
    struct json_object *issued_at_seconds;
    struct json_object *expires_at_seconds;

    parsed_response = json_tokener_parse(response_buffer);
    if (parsed_response == NULL) {
        // JSON parsing failed
        console(CONSOLE_ERROR, "failed to parse access key JSON data");
        json_object_put(parsed_response);
        free(response_buffer);
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
        free(response_buffer);
        return false;
    }

    access_key.public_key = malloc(strlen(json_object_get_string(public_key)) + 1); // +1 for null-terminator
    strcpy(access_key.public_key, json_object_get_string(public_key));
    access_key.issued_at_seconds = json_object_get_int64(issued_at_seconds);
    access_key.expires_at_seconds = json_object_get_int64(expires_at_seconds);

    json_object_put(parsed_response);
    free(response_buffer);
    return true;
};

void access_task(Scheduler *sch) {
    console(CONSOLE_DEBUG, "access task");

    if (check_access_key_near_expiration()) {
        request_access_key();
        write_access_key();
    } else {
        console(CONSOLE_DEBUG, "key is still valid");
    }

    // Schedule the next key request
    schedule_task(sch, time(NULL) + config.access_interval, access_task, "access");
}

void access_service(Scheduler *sch) {
    if (read_access_key()) {
        if (check_access_key_near_expiration()) {
            request_access_key();
            write_access_key();
        }
    } else {
        request_access_key();
        write_access_key();
    }

    console(CONSOLE_DEBUG, "access service");
    schedule_task(sch, time(NULL) + config.access_interval, access_task, "access");
}

void clean_access_service() {
    if (access_key.public_key != NULL) {
        free(access_key.public_key);
        access_key.public_key = NULL;
    }
}
