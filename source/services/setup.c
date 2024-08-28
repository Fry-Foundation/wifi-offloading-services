#include "setup.h"
#include "lib/console.h"
#include "lib/http-requests.h"
#include "services/access.h"
#include "services/config.h"
#include "services/device_status.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "services/device_info.h"
#include <json-c/json.h>

#define SETUP_ENDPOINT "/api/nfNode/setup"
#define SETUP_COMPLETE_ENDPOINT "/api/nfNode/setup/complete"

// Backend should handle setup requests that have already been created for this access key
// If no setup request exists, create one
void request_setup(DeviceInfo *device_info) {
    console(CONSOLE_DEBUG, "Request setup");
    console(CONSOLE_DEBUG, "Access key: %s", access_key.public_key);

    // Build setup URL
    char setup_url[256];
    snprintf(setup_url, sizeof(setup_url), "%s%s", config.main_api, SETUP_ENDPOINT);
    console(CONSOLE_DEBUG, "setup_url: %s", setup_url);

    //Request body
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
    HttpPostOptions setup_options = {
        .url = setup_url,
        .legacy_key = access_key.public_key,
        .body_json_str = body,
    };

    HttpResult result = http_post(&setup_options);
    json_object_put(json_body);
    
    if (result.is_error) {
        console(CONSOLE_ERROR, "setup request failed: %s", result.error);
        return;
    }

    if (result.response_buffer == NULL) {
        console(CONSOLE_ERROR, "setup request failed: no response received");
        return;
    }

    console(CONSOLE_DEBUG, "setup request response: %s", result.response_buffer);
}

void setup_task(Scheduler *sch, void *task_context, DeviceInfo *device_info) {
    (void)task_context;

    if (device_status == Unknown) {
        // Schedule setup_task to rerun later
        // The device's status has to be defined beforehand
        console(CONSOLE_DEBUG, "device status is Unknown, rescheduling setup task");
        schedule_task(sch, time(NULL) + config.setup_interval, setup_task, "setup", NULL);
    }

    if (device_status == Initial) {
        console(CONSOLE_DEBUG, "requesting setup");
        request_setup(device_info);
    }
}

void setup_service(Scheduler *sch, DeviceInfo *device_info) { setup_task(sch, NULL, device_info); }
