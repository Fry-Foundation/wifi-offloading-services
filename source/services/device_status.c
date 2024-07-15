#include "device_status.h"
#include "lib/scheduler.h"
#include "lib/console.h"
#include "lib/curl_helpers.h"
#include "services/config.h"
#include "services/access.h"
#include <stdbool.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <json-c/json.h>

#define DEVICE_STATUS_ENDPOINT "/api/nfnode/device-status"

DeviceStatus device_status = Unknown;

bool on_boot = true;

DeviceStatus request_device_status() { 
    CURL *curl;
    CURLcode res;
    
    char *response_buffer = init_response_buffer();

    curl = curl_easy_init();

    if (!curl) {
        console(CONSOLE_ERROR, "device status curl could not be initialized");
        free(response_buffer);
        return Unknown;
    }

    // Set up as POST request
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    // Url
    char device_status_url[256];
    snprintf(device_status_url, sizeof(device_status_url), "%s%s", config.main_api, DEVICE_STATUS_ENDPOINT);
    curl_easy_setopt(curl, CURLOPT_URL, device_status_url);

    // Request headers
    struct curl_slist *headers = NULL;
    char public_key_header[1024];
    snprintf(public_key_header, sizeof(public_key_header), "public_key: %s", access_key.public_key);
    headers = curl_slist_append(headers, public_key_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Request body
    json_object *json_body = json_object_new_object();
    json_object_object_add(json_body, "on_boot", json_object_new_boolean(on_boot));
    const char *body = json_object_to_json_string(json_body);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);

    console(CONSOLE_DEBUG, "device status request body %s", body);

    // Response callback and buffer
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, save_to_buffer_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);

    res = curl_easy_perform(curl);

    json_object_put(json_body);

    if (res != CURLE_OK) {
        console(CONSOLE_ERROR, "access key curl request failed:", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        free(response_buffer);
        return Unknown;
    }

    // Parse response
    struct json_object *parsed_response;
    struct json_object *device_status;

    parsed_response = json_tokener_parse(response_buffer);
    if (parsed_response == NULL) {
        // JSON parsing failed
        console(CONSOLE_ERROR, "failed to parse device status JSON data");
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        json_object_put(parsed_response);
        free(response_buffer);
        return Unknown;
    }

    if (!json_object_object_get_ex(parsed_response, "deviceStatus", &device_status)) {
        console(CONSOLE_ERROR, "publicKey field missing or invalid");
        json_object_put(parsed_response);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        free(response_buffer);
        return Unknown;
    }

    int response_device_status = json_object_get_int64(device_status);

    json_object_put(parsed_response);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    free(response_buffer);

    console(CONSOLE_DEBUG, "device status response: %d", response_device_status);

    on_boot = false;

    return response_device_status; 
}

void device_status_task(Scheduler *sch) {
    device_status = request_device_status();
    console(CONSOLE_DEBUG, "device status: %d", device_status);
    console(CONSOLE_DEBUG, "device status interval: %d", config.device_status_interval);
    console(CONSOLE_DEBUG, "device status interval time: %ld", time(NULL) + config.device_status_interval);
    schedule_task(sch, time(NULL) + config.device_status_interval, device_status_task, "device status");
}

void device_status_service(Scheduler *sch) {
    device_status_task(sch);

    // Side effects
    // Make sure wayru operator is running (all status codes but 6)
    // Start the peaq did service (on status 5)
    // Check that the captive portal is running (on status 6)
    // Disable wayru operator network (on status 6)
}
