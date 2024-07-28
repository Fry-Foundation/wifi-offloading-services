#include "registration.h"
#include "lib/console.h"
#include "lib/http-requests.h"
#include "services/config.h"
#include <json-c/json.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define REGISTER_ENDPOINT "access/register"
#define DEVICE_REGISTRATION_FILE "registration.json"

// Writes a json file with the device registration
bool save_device_registration(char *device_registration_json) {
    char registration_file_path[256];
    snprintf(registration_file_path, sizeof(registration_file_path), "%s/%s", config.data_path,
             DEVICE_REGISTRATION_FILE);

    FILE *file = fopen(registration_file_path, "w");
    if (file == NULL) {
        console(CONSOLE_ERROR, "failed to open device registration file");
        return false;
    }

    fprintf(file, "%s", device_registration_json);
    fclose(file);

    return true;
}

// Reads a json file and returns the device registration
char *read_device_registration() {
    char registration_file_path[256];
    snprintf(registration_file_path, sizeof(registration_file_path), "%s/%s", config.data_path,
             DEVICE_REGISTRATION_FILE);

    FILE *file = fopen(registration_file_path, "r");
    if (file == NULL) {
        console(CONSOLE_ERROR, "failed to open device registration file");
        return NULL;
    }

    // Get file size to allocate memory
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *device_registration = malloc(file_size + 1);
    if (device_registration == NULL) {
        console(CONSOLE_ERROR, "failed to allocate memory for device registration");
        fclose(file);
        return NULL;
    }

    // Read file
    fread(device_registration, 1, file_size, file);
    device_registration[file_size] = '\0';

    fclose(file);

    return device_registration;
}

Registration parse_device_registration(const char *device_registration_json) {
    struct json_object *parsed_registration;
    struct json_object *wayru_device_id;
    struct json_object *access_key;

    parsed_registration = json_tokener_parse(device_registration_json);
    if (parsed_registration == NULL) {
        // JSON parsing failed
        console(CONSOLE_ERROR, "failed to parse device registration JSON data");
        return (Registration){.wayru_device_id = NULL, .access_key = NULL};
    }

    if (!json_object_object_get_ex(parsed_registration, "wayru_device_id", &wayru_device_id)) {
        console(CONSOLE_ERROR, "failed to get wayru_device_id from device registration");
        json_object_put(parsed_registration);
        return (Registration){.wayru_device_id = NULL, .access_key = NULL};
    }

    if (!json_object_object_get_ex(parsed_registration, "access_key", &access_key)) {
        console(CONSOLE_ERROR, "failed to get access_key from device registration");
        json_object_put(parsed_registration);
        return (Registration){.wayru_device_id = NULL, .access_key = NULL};
    }

    Registration registration = {
        .wayru_device_id = strdup(json_object_get_string(wayru_device_id)),
        .access_key = strdup(json_object_get_string(access_key)),
    };

    json_object_put(parsed_registration);

    return registration;
}

bool check_registration() {
    char *registration_str = read_device_registration();
    if (registration_str == NULL) {
        console(CONSOLE_INFO, "device not registered");
        return false;
    }

    Registration registration = parse_device_registration(registration_str);
    if (registration.wayru_device_id == NULL || registration.access_key == NULL) {
        console(CONSOLE_INFO, "device not registered");
        free(registration_str);
        return false;
    }

    console(CONSOLE_INFO, "device registered");
    console(CONSOLE_INFO, "wayru_device_id: %s", registration.wayru_device_id);
    console(CONSOLE_INFO, "access_key: %s", registration.access_key);
    free(registration_str);
    free(registration.wayru_device_id);
    free(registration.access_key);
    return true;
}

bool init_registration(char *mac, char *model, char *brand) {
    if (check_registration()) {
        return true;
    }

    // Url
    char register_url[256];
    snprintf(register_url, sizeof(register_url), "%s/%s", config.accounting_api, REGISTER_ENDPOINT);

    // Request body
    json_object *json_body = json_object_new_object();
    json_object_object_add(json_body, "mac", json_object_new_string(mac));
    json_object_object_add(json_body, "model", json_object_new_string(model));
    json_object_object_add(json_body, "brand", json_object_new_string(brand));
    const char *body = json_object_to_json_string(json_body);
    console(CONSOLE_DEBUG, "register device request body %s", body);

    HttpPostOptions options = {
        .url = register_url,
        .body_json_str = body,
    };

    HttpResult result = http_post(&options);
    json_object_put(json_body);

    if (result.is_error) {
        console(CONSOLE_ERROR, "failed to register device");
        console(CONSOLE_ERROR, "error: %s", result.error);
        return false;
    }

    if (result.response_buffer == NULL) {
        console(CONSOLE_ERROR, "failed to register device");
        console(CONSOLE_ERROR, "no response received");
        return false;
    }

    // Parse response
    Registration registration = parse_device_registration(result.response_buffer);
    if (registration.wayru_device_id == NULL || registration.access_key == NULL) {
        console(CONSOLE_ERROR, "failed to register device");
        free(result.response_buffer);
        return false;
    }

    // Save registration
    bool write_result = save_device_registration(result.response_buffer);

    // Cleanup
    clean_registration(&registration);
    free(result.response_buffer);

    return write_result;
}

// Returns the device registration
Registration get_registration() {
    char *registration_str = read_device_registration();
    if (registration_str == NULL) {
        return (Registration){.wayru_device_id = NULL, .access_key = NULL};
    }

    Registration registration = parse_device_registration(registration_str);
    free(registration_str);
    return registration;
}

void clean_registration(Registration *registration) {
    if (registration == NULL) {
        return;
    }

    if (registration->wayru_device_id != NULL) {
        free(registration->wayru_device_id);
        registration->wayru_device_id = NULL;
    }

    if (registration->access_key != NULL) {
        free(registration->access_key);
        registration->access_key = NULL;
    }
}
