#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <json-c/json.h>
#include "access.h"
#include "setup.h"
#include "accounting.h"
#include "../store/config.h"
#include "../store/state.h"
#include "../utils/requests.h"
#include "../utils/script_runner.h"

#define KEY_FILE "/data/access-key"
#define KEY_FILE_BUFFER_SIZE 768
#define REQUEST_BODY_BUFFER_SIZE 256
#define MAX_KEY_SIZE 512
#define MAX_TIMESTAMP_SIZE 256
#define ACCESS_ENDPOINT "/api/nfnode/access"
#define SCRIPTS_PATH "/etc/wayru-os-services/scripts"

time_t convert_to_time_t(char *timestamp_str)
{
    long long int epoch = strtoll(timestamp_str, NULL, 10);
    return (time_t)epoch;
}

AccessKey *init_access_key()
{
    AccessKey *access_key = malloc(sizeof(AccessKey));
    access_key->key = NULL;
    access_key->created_at = 0;
    access_key->expires_at = 0;

    read_access_key(access_key);

    return access_key;
}

int read_access_key(AccessKey *access_key)
{
    printf("[access] reading stored access key\n");

    char key_file_path[KEY_FILE_BUFFER_SIZE];
    snprintf(key_file_path, sizeof(key_file_path), "%s%s", getConfig().basePath, KEY_FILE);

    FILE *file = fopen(key_file_path, "r");
    if (file == NULL)
    {
        // Handle error (e.g., file not found)
        fprintf(stderr, "Failed to open key file.\n");
        return 0;
    }

    char line[512];
    char public_key[MAX_KEY_SIZE];
    char created_at[MAX_TIMESTAMP_SIZE];
    char expires_at[MAX_TIMESTAMP_SIZE];

    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "public_key", 10) == 0)
        {
            // Subtract the length of "public_key" from the total length
            size_t key_length = strlen(line) - 11;
            access_key->key = malloc(key_length + 1);
            if (access_key->key == NULL)
            {
                perror("Failed to allocate memory for key");
                fclose(file);
                return 0;
            }
            strcpy(access_key->key, line + 11);
            access_key->key[key_length] = '\0';
        }
        else if (strncmp(line, "created_at", 10) == 0)
        {
            sscanf(line, "created_at %s", created_at);
        }
        else if (strncmp(line, "expires_at", 10) == 0)
        {
            sscanf(line, "expires_at %s", expires_at);
        }
    }

    fclose(file);

    access_key->created_at = convert_to_time_t(created_at);
    access_key->expires_at = convert_to_time_t(expires_at);

    return 1;
}

void write_access_key(AccessKey *access_key)
{
    printf("[access] writing new access key\n");

    char keyFile[KEY_FILE_BUFFER_SIZE];
    snprintf(keyFile, sizeof(keyFile), "%s%s", getConfig().basePath, KEY_FILE);

    FILE *file = fopen(keyFile, "w");
    if (file == NULL)
    {
        printf("Unable to open file for writing\n");
        return;
    }

    fprintf(file, "public_key %s\n", access_key->key);
    fprintf(file, "created_at %ld\n", access_key->created_at);
    fprintf(file, "expires_at %ld\n", access_key->expires_at);

    fclose(file);
}

void process_access_status(char *status)
{
    printf("[access] Processing access status\n");
    if (strcmp(status, "initial") == 0)
    {
        state.access_status = 0;
    }
    else if (strcmp(status, "setup-pending") == 0)
    {
        state.access_status = 1;
    }
    else if (strcmp(status, "setup-approved") == 0)
    {
        state.access_status = 2;
    }
    else if (strcmp(status, "mint-pending") == 0)
    {
        state.access_status = 3;
    }
    else if (strcmp(status, "ready") == 0)
    {
        state.access_status = 4;
    }
    else if (strcmp(status, "banned") == 0)
    {
        state.access_status = 5;
    }
    else
    {
        printf("[access] Unknown access status: %s\n", status);
    }
}

size_t process_access_key_response(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t realsize = size * nmemb;
    AccessKey *access_key = (AccessKey *)userdata;

    fprintf(stderr, "[access] Received JSON data: %s\n", ptr);

    // Parse JSON
    struct json_object *parsed_response;
    struct json_object *public_key;
    struct json_object *status;
    struct json_object *payload;
    struct json_object *iat;
    struct json_object *exp;

    parsed_response = json_tokener_parse(ptr);
    if (parsed_response == NULL)
    {
        // JSON parsing failed
        fprintf(stderr, "Failed to parse JSON\n");
        return realsize;
    }

    // Extract fields
    // Enhanced error logging
    bool error_occurred = false;
    if (!json_object_object_get_ex(parsed_response, "publicKey", &public_key))
    {
        fprintf(stderr, "[access] error: publicKey field missing or invalid\n");
        error_occurred = true;
    }
    if (!json_object_object_get_ex(parsed_response, "status", &status))
    {
        fprintf(stderr, "[access] error: status field missing or invalid\n");
        error_occurred = true;
    }
    if (!json_object_object_get_ex(parsed_response, "payload", &payload))
    {
        fprintf(stderr, "[access] error: payload field missing or invalid\n");
        error_occurred = true;
    }
    if (payload && !json_object_object_get_ex(payload, "iat", &iat))
    {
        fprintf(stderr, "[access] error: iat field missing or invalid in payload\n");
        error_occurred = true;
    }
    if (payload && !json_object_object_get_ex(payload, "exp", &exp))
    {
        fprintf(stderr, "[access] error: exp field missing or invalid in payload\n");
        error_occurred = true;
    }

    if (error_occurred)
    {
        json_object_put(parsed_response);
        return realsize;
    }

    access_key->key = malloc(strlen(json_object_get_string(public_key)) + 1); // +1 for null-terminator
    strcpy(access_key->key, json_object_get_string(public_key));
    access_key->created_at = json_object_get_int64(iat);
    access_key->expires_at = json_object_get_int64(exp);

    char *status_value = malloc(strlen(json_object_get_string(status)) + 1);
    strcpy(status_value, json_object_get_string(status));
    printf("[access] status: %s\n", status_value);
    process_access_status(status_value);

    json_object_put(parsed_response);
    return realsize;
}

int check_access_key_near_expiration(AccessKey *access_key)
{
    printf("[access] Checking if key is near expiration\n");
    time_t now;
    time(&now);

    if (difftime(access_key->expires_at, now) <= 600)
    {
        printf("[access] Key is near expiration\n");
        return 1;
    }
    else
    {
        printf("[access] Key is not near expiration\n");
        return 0;
    }
}

int request_access_key(AccessKey *access_key)
{
    printf("[access] Request access key\n");

    json_object *json_data = json_object_new_object();

    json_object_object_add(json_data, "device_id", json_object_new_string(getConfig().deviceId));
    json_object_object_add(json_data, "mac", json_object_new_string(getConfig().mac));
    json_object_object_add(json_data, "name", json_object_new_string(getConfig().name));
    json_object_object_add(json_data, "brand", json_object_new_string(getConfig().brand));
    json_object_object_add(json_data, "model", json_object_new_string(getConfig().model));
    json_object_object_add(json_data, "public_ip", json_object_new_string(getConfig().public_ip));
    json_object_object_add(json_data, "os_name", json_object_new_string(getConfig().os_name));
    json_object_object_add(json_data, "os_version", json_object_new_string(getConfig().osVersion));
    json_object_object_add(json_data, "os_services_version", json_object_new_string(getConfig().servicesVersion));
    json_object_object_add(json_data, "on_boot", json_object_new_string(state.onBoot == 1 ? "true" : "false"));

    const char *json_data_string = json_object_to_json_string(json_data);
    printf("[access] DeviceData -> %s\n", json_data_string);

    // Build access URL
    char access_url[256];
    snprintf(access_url, sizeof(access_url), "%s%s", getConfig().main_api, ACCESS_ENDPOINT);
    printf("[access] access_url: %s\n", access_url);

    // Request options
    PostRequestOptions options = {
        .url = access_url,
        .body = json_data_string,
        .filePath = NULL,
        .key = NULL,
        .writeFunction = process_access_key_response,
        .writeData = access_key};

    int result_post = performHttpPost(&options);
    json_object_put(json_data);

    if (result_post == 1)
    {
        printf("[access] Request was successful.\n");
        return 1;
    }
    else
    {
        printf("[access] Request failed.\n");
        return 0;
    }
};

void disable_default_wireless_network()
{
    if (getConfig().devEnv == 1)
    {
        printf("[access] Not disabling default wireless network in dev environment\n");
        return;
    }

    printf("[access] Disabling default wireless network\n");
    
    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s%s", SCRIPTS_PATH, "/disable-default-wireless.sh");
    
    char *disable_output = run_script(script_file);
    printf("[access] disable_output: %s\n", disable_output);
    free(disable_output);
}

void configure_with_access_status(int access_status)
{
    printf("[access] Configuring with access status\n");
    if (access_status == 0)
    {
        printf("[access] Access status is 'initial'\n");
        state.setup = 1;
        state.accounting = 0;
        stop_opennds();
    }
    else if (access_status == 1)
    {
        printf("[access] Access status is 'setup-pending'\n");
        state.setup = 0;
        state.accounting = 0;
        stop_opennds();
    }
    else if (access_status == 2)
    {
        printf("[access] Access status is 'setup-approved'\n");
        state.setup = 0;
        state.accounting = 0;
        completeSetup();
        stop_opennds();
    }
    else if (access_status == 3)
    {
        printf("[access] Access status is 'mint-pending'\n");
        state.setup = 0;
        state.accounting = 0;
        stop_opennds();
    }
    else if (access_status == 4)
    {
        printf("[access] Access status is 'ready'\n");
        state.setup = 0;
        state.accounting = 1;
        start_opennds();

        // @TODO: Disable Wayru Operator network
    } else if (access_status == 5) {
        printf("[access] Access status is 'banned'\n");
        state.setup = 0;
        state.accounting = 0;
        stop_opennds();
    }
}

void access_task()
{
    printf("[access] Access task\n");

    int is_expired = check_access_key_near_expiration(state.access_key);
    if (is_expired == 1 || state.access_key->key == NULL)
    {
        request_access_key(state.access_key);
        write_access_key(state.access_key);
        configure_with_access_status(state.access_status);
    }
    else
    {
        printf("[access] key is still valid\n");
    }
}