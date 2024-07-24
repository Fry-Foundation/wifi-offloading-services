#include "lib/console.h"
#include "lib/http-requests.h"
#include "services/access.h"
#include "services/config.h"
#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

#define DEV_PATH "."
#define OPENWRT_PATH "/etc/wayru-os-services"
#define DATA_PATH "/data"
#define END_REPORT_PATH "/end-report"
#define END_REPORT_ENDPOINT "/gateways/connections/end"

char data_path[256];
char end_report_path[256];

json_object *load_end_report(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        console(CONSOLE_ERROR, "failed to open file with end report");
        return NULL;
    }

    json_object *mac_array = json_object_new_array();
    if (mac_array == NULL) {
        fclose(file);
        console(CONSOLE_ERROR, "failed to create end report JSON array");
        return NULL;
    }

    char line[20];
    while (fgets(line, sizeof(line), file) != NULL) {
        // Remove any trailing newline characters
        line[strcspn(line, "\r\n")] = '\0';

        // Add the MAC address to the JSON array
        json_object_array_add(mac_array, json_object_new_string(line));
    }

    fclose(file);
    return mac_array;
}

size_t process_end_report_response(char *ptr, size_t size, size_t nmemb, void *userdata) {
    // Calculate the number of bytes received
    size_t num_bytes = size * nmemb;

    // Check if the response is "ack"
    if (strncmp(ptr, "ack", num_bytes) == 0) {
        console(CONSOLE_DEBUG, "received ack from server");
    } else {
        console(CONSOLE_ERROR, "unexpected response from server: %.*s", (int)num_bytes, ptr);
    }

    // Return the number of bytes processed
    return num_bytes;
}

void post_end_report(json_object *mac_address_array) {
    console(CONSOLE_DEBUG, "posting end report");

    // Build end report url
    char end_report_url[256];
    snprintf(end_report_url, sizeof(end_report_url), "%s%s", config.accounting_api, END_REPORT_ENDPOINT);
    console(CONSOLE_DEBUG, "end_report_url: %s", end_report_url);

    // Stringify the JSON
    const char *mac_address_json = json_object_to_json_string(mac_address_array);

    HttpPostOptions post_end_report_options = {
        .url = end_report_url,
        .body_json_str = mac_address_json,
    };

    HttpResult result = http_post(&post_end_report_options);
    if (result.is_error) {
        console(CONSOLE_ERROR, "failed to post end report: %s", result.error);
        return;
    }

    if (result.response_buffer == NULL) {
        console(CONSOLE_ERROR, "failed to post end report: no response received");
        return;
    }

    console(CONSOLE_DEBUG, "end report response: %s", result.response_buffer);
}

void end_report_task() {
    int dev_env = config.dev_env;

    console(CONSOLE_DEBUG, "dev_env: %d", dev_env);

    // Set up paths
    char base_path[256];
    if (dev_env == 1) {
        strncpy(base_path, DEV_PATH, sizeof(base_path));
        base_path[sizeof(base_path) - 1] = '\0'; // Ensure null termination
    } else {
        strncpy(base_path, OPENWRT_PATH, sizeof(base_path));
        base_path[sizeof(base_path) - 1] = '\0'; // Ensure null termination
    }
    snprintf(data_path, sizeof(data_path), "%s%s", base_path, DATA_PATH);
    console(CONSOLE_DEBUG, "data_path: %s", data_path);

    snprintf(end_report_path, sizeof(end_report_path), "%s%s", data_path, END_REPORT_PATH);
    console(CONSOLE_DEBUG, "end_report_path: %s", end_report_path);

    json_object *mac_address_array = load_end_report(end_report_path);

    if (mac_address_array != NULL) {
        console(CONSOLE_DEBUG, "loaded MAC addresses:\n%s", json_object_to_json_string(mac_address_array));

        // Post to server
        post_end_report(mac_address_array);

        // Free the JSON object
        json_object_put(mac_address_array);
    }
}
