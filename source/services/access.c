#include "access.h"
#include "lib/console.h"
#include "lib/requests.h"
#include "lib/scheduler.h"
#include "lib/script_runner.h"
#include "services/config.h"
#include "services/device_data.h"
#include "services/setup.h"
#include "services/state.h"
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
#define MAX_KEY_SIZE 512
#define MAX_TIMESTAMP_SIZE 256
#define ACCESS_ENDPOINT "/api/nfnode/access-v2"
#define SCRIPTS_PATH "/etc/wayru-os-services/scripts"

AccessKey access_key = {NULL};

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

    char line[512];
    char public_key[MAX_KEY_SIZE];
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

// void process_access_status(char *status) {
//     console(CONSOLE_DEBUG, "Processing access status");
//     if (strcmp(status, "initial") == 0) {
//         state.access_status = 0;
//     } else if (strcmp(status, "setup-pending") == 0) {
//         state.access_status = 1;
//     } else if (strcmp(status, "setup-approved") == 0) {
//         state.access_status = 2;
//     } else if (strcmp(status, "mint-pending") == 0) {
//         state.access_status = 3;
//     } else if (strcmp(status, "ready") == 0) {
//         state.access_status = 4;
//     } else if (strcmp(status, "banned") == 0) {
//         state.access_status = 5;
//     } else {
//         console(CONSOLE_DEBUG, "Unknown access status: %s", status);
//     }
// }

size_t process_access_key_response(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t realsize = size * nmemb;

    console(CONSOLE_DEBUG, "received access key JSON data");

    // Parse JSON
    struct json_object *parsed_response;
    struct json_object *public_key;
    struct json_object *issued_at_seconds;
    struct json_object *expires_at_seconds;

    parsed_response = json_tokener_parse(ptr);
    if (parsed_response == NULL) {
        // JSON parsing failed
        console(CONSOLE_ERROR, "failed to parse access key JSON data");
        return realsize;
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
        return realsize;
    }

    access_key.public_key =
        malloc(strlen(json_object_get_string(public_key)) + 1); // +1 for null-terminator
    strcpy(access_key.public_key, json_object_get_string(public_key));
    access_key.issued_at_seconds = json_object_get_int64(issued_at_seconds);
    access_key.expires_at_seconds = json_object_get_int64(expires_at_seconds);

    json_object_put(parsed_response);
    return realsize;
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

int request_access_key() {
    console(CONSOLE_DEBUG, "request access key");

    json_object *json_data = json_object_new_object();

    json_object_object_add(json_data, "device_id", json_object_new_string(device_data.device_id));
    json_object_object_add(json_data, "mac", json_object_new_string(device_data.mac));
    json_object_object_add(json_data, "name", json_object_new_string(device_data.name));
    json_object_object_add(json_data, "brand", json_object_new_string(device_data.brand));
    json_object_object_add(json_data, "model", json_object_new_string(device_data.model));
    json_object_object_add(json_data, "public_ip", json_object_new_string(device_data.public_ip));
    json_object_object_add(json_data, "os_name", json_object_new_string(device_data.os_name));
    json_object_object_add(json_data, "os_version", json_object_new_string(device_data.os_version));
    json_object_object_add(json_data, "os_services_version",
                           json_object_new_string(device_data.os_services_version));
    json_object_object_add(json_data, "on_boot",
                           json_object_new_string(state.on_boot == 1 ? "true" : "false"));

    const char *json_data_string = json_object_to_json_string(json_data);
    console(CONSOLE_DEBUG, "device data -> %s", json_data_string);

    // Build access URL
    char access_url[256];
    snprintf(access_url, sizeof(access_url), "%s%s", config.main_api, ACCESS_ENDPOINT);
    console(CONSOLE_DEBUG, "access_url: %s", access_url);

    // Request options
    PostRequestOptions options = {.url = access_url,
                                  .body = json_data_string,
                                  .filePath = NULL,
                                  .key = NULL,
                                  .writeFunction = process_access_key_response,
                                  .writeData = access_key.public_key};

    int result_post = performHttpPost(&options);
    json_object_put(json_data);

    if (result_post == 1) {
        console(CONSOLE_DEBUG, "access request was successful.");
        return 1;
    } else {
        console(CONSOLE_DEBUG, "access request failed.");
        return 0;
    }
};

// void disable_default_wireless_network() {
//     if (config.dev_env) {
//         console(CONSOLE_DEBUG, "not disabling default wireless network in dev environment");
//         return;
//     }
//
//     if (state.already_disabled_wifi == 1) {
//         console(CONSOLE_DEBUG, "default wireless network already disabled");
//         return;
//     }
//
//     console(CONSOLE_DEBUG, "disabling default wireless network");
//
//     char script_file[256];
//     snprintf(script_file, sizeof(script_file), "%s%s", SCRIPTS_PATH,
//              "/disable-default-wireless.sh");
//
//     char *disable_output = run_script(script_file);
//     console(CONSOLE_DEBUG, "disable_output: %s", disable_output);
//
//     state.already_disabled_wifi = 1;
//
//     free(disable_output);
//     sleep(10); // Wait for the network to be disabled
// }

// void configure_with_access_status(int access_status) {
//     console(CONSOLE_DEBUG, "configuring with access status");
//     if (access_status == 0) {
//         console(CONSOLE_DEBUG, "access status is 'initial'");
//         state.setup = 1;
//         state.accounting = 0;
//         // stop_opennds();
//     } else if (access_status == 1) {
//         console(CONSOLE_DEBUG, "access status is 'setup-pending'");
//         state.setup = 0;
//         state.accounting = 0;
//         // stop_opennds();
//     } else if (access_status == 2) {
//         console(CONSOLE_DEBUG, "access status is 'setup-approved'");
//         state.setup = 0;
//         state.accounting = 0;
//         completeSetup();
//         // stop_opennds();
//     } else if (access_status == 3) {
//         console(CONSOLE_DEBUG, "access status is 'mint-pending'");
//         state.setup = 0;
//         state.accounting = 0;
//         // stop_opennds();
//     } else if (access_status == 4) {
//         console(CONSOLE_DEBUG, "access status is 'ready'");
//         state.setup = 0;
//         state.accounting = 1;
//
//         // disable_default_wireless_network();
//         // start_opennds();
//
//         // peaq_id_task();
//     } else if (access_status == 5) {
//         console(CONSOLE_DEBUG, "access status is 'banned'");
//         state.setup = 0;
//         state.accounting = 1;
//         // stop_opennds();
//     }
// }

void access_task() {
    console(CONSOLE_DEBUG, "access task");

    if (check_access_key_near_expiration()) {
        request_access_key();
        write_access_key();
    } else {
        console(CONSOLE_DEBUG, "key is still valid");
    }

    // Schedule the next key request
    schedule_task(time(NULL) + config.access_task_interval, access_task, NULL, "access task");
}

void init_access_service() {
    access_key.public_key = NULL;
    access_key.issued_at_seconds = 0;
    access_key.expires_at_seconds = 0;

    if (read_access_key()) {
        if (check_access_key_near_expiration()) {
            request_access_key();
            write_access_key();
        }
    } else {
        request_access_key();
        write_access_key();
    }

    // Schedule the next key request
    schedule_task(time(NULL) + config.access_task_interval, access_task, NULL, "access task");
}

void clean_access_service() {
    if (access_key.public_key != NULL) {
        free(access_key.public_key);
        access_key.public_key = NULL;
    }
}
