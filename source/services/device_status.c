#include "device_status.h"
#include "lib/console.h"
#include "lib/http-requests.h"
#include "lib/scheduler.h"
#include "services/access.h"
#include "services/config.h"
#include <json-c/json.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define DEVICE_STATUS_ENDPOINT "/api/nfnode/device-status"

DeviceStatus device_status = Unknown;

bool on_boot = true;

DeviceStatus request_device_status() {
    // Url
    char device_status_url[256];
    snprintf(device_status_url, sizeof(device_status_url), "%s%s", config.main_api, DEVICE_STATUS_ENDPOINT);

    // Request body
    json_object *json_body = json_object_new_object();
    json_object_object_add(json_body, "on_boot", json_object_new_boolean(on_boot));
    const char *body = json_object_to_json_string(json_body);
    console(CONSOLE_DEBUG, "device status request body %s", body);

    HttpPostOptions options = {
        .url = device_status_url,
        .legacy_key = access_key.public_key,
        .body_json_str = body,
    };

    HttpResult result = http_post(&options);

    json_object_put(json_body);

    if (result.is_error) {
        console(CONSOLE_ERROR, "failed to request device status");
        console(CONSOLE_ERROR, "error: %s", result.error);
        return Unknown;
    }

    if (result.response_buffer == NULL) {
        console(CONSOLE_ERROR, "failed to request device status");
        console(CONSOLE_ERROR, "no response received");
        return Unknown;
    }

    // Parse response
    struct json_object *parsed_response;
    struct json_object *device_status;

    parsed_response = json_tokener_parse(result.response_buffer);
    if (parsed_response == NULL) {
        // JSON parsing failed
        console(CONSOLE_ERROR, "failed to parse device status JSON data");
        free(result.response_buffer);
        return Unknown;
    }

    if (!json_object_object_get_ex(parsed_response, "deviceStatus", &device_status)) {
        console(CONSOLE_ERROR, "publicKey field missing or invalid");
        json_object_put(parsed_response);
        free(result.response_buffer);
        return Unknown;
    }

    int response_device_status = json_object_get_int64(device_status);

    json_object_put(parsed_response);
    free(result.response_buffer);

    console(CONSOLE_DEBUG, "device status response: %d", response_device_status);

    on_boot = false;

    return response_device_status;
}

void device_status_task(Scheduler *sch, void *task_context) {
    (void)task_context;

    device_status = request_device_status();
    console(CONSOLE_DEBUG, "device status: %d", device_status);
    console(CONSOLE_DEBUG, "device status interval: %d", config.device_status_interval);
    console(CONSOLE_DEBUG, "device status interval time: %ld", time(NULL) + config.device_status_interval);
    schedule_task(sch, time(NULL) + config.device_status_interval, device_status_task, "device status", NULL);
}

void device_status_service(Scheduler *sch) {
    device_status_task(sch, NULL);

    // Side effects
    // Make sure wayru operator is running (all status codes but 6)
    // Start the peaq did service (on status 5)
    // Check that the captive portal is running (on status 6)
    // Disable wayru operator network (on status 6)
}
