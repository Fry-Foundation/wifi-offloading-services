#include "../store/config.h"
#include "../store/state.h"
#include "../utils/requests.h"
#include <json-c/json.h>
#include <stdio.h>

#define DEV_PATH "."
#define OPENWRT_PATH "/etc/wayru-os-services"
#define DATA_PATH "/data"
#define END_REPORT_PATH "/end-report"
#define END_REPORT_ENDPOINT "/gateways/connections/end"

char data_path[256];
char end_report_path[256];

json_object *load_end_report(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return NULL;
    }

    json_object *mac_array = json_object_new_array();
    if (mac_array == NULL)
    {
        fclose(file);
        fprintf(stderr, "Error creating JSON array\n");
        return NULL;
    }

    char line[20];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        // Remove any trailing newline characters
        line[strcspn(line, "\r\n")] = '\0';

        // Add the MAC address to the JSON array
        json_object_array_add(mac_array, json_object_new_string(line));
    }

    fclose(file);
    return mac_array;
}

size_t process_end_report_response(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    // Calculate the number of bytes received
    size_t num_bytes = size * nmemb;

    // Check if the response is "ack"
    if (strncmp(ptr, "ack", num_bytes) == 0) {
        printf("Received acknowledgment from server\n");
    } else {
        fprintf(stderr, "Unexpected response from server: %.*s\n", (int)num_bytes, ptr);
    }

    // Return the number of bytes processed
    return num_bytes;
}

void post_end_report(json_object *mac_address_array)
{
    printf("[end_report] posting end report\n");

    // Build end report url
    char end_report_url[256];
    snprintf(end_report_url, sizeof(end_report_url), "%s%s", getConfig().accounting_api, END_REPORT_ENDPOINT);
    printf("[end_report] end_report_url: %s\n", end_report_url);

    // Stringify the JSON
    const char *mac_address_json = json_object_to_json_string(mac_address_array);

    // Request options
    PostRequestOptions post_end_report_options = {
        .url = end_report_url,
        .key = state.access_key->key,
        .body = mac_address_json,
        .filePath = NULL,
        .writeFunction = process_end_report_response,
        .writeData = NULL,
    };

    performHttpPost(&post_end_report_options);
}

void end_report_task()
{
    int dev_env = getConfig().devEnv;

    printf("[end_report] dev_env: %d\n", dev_env);

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
    printf("[end_report] data_path: %s\n", data_path);

    snprintf(end_report_path, sizeof(end_report_path), "%s%s", data_path, END_REPORT_PATH);
    printf("[end_report] end_report_path: %s\n", end_report_path);    

    json_object *mac_address_array = load_end_report(end_report_path);

    if (mac_address_array != NULL)
    {
        printf("Loaded MAC addresses:\n%s\n", json_object_to_json_string(mac_address_array));

        // Post to server
        post_end_report(mac_address_array);

        // Free the JSON object
        json_object_put(mac_address_array);
    }
}