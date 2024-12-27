#include "setup.h"
#include "lib/console.h"
#include "lib/http-requests.h"
#include "services/config.h"
#include "services/device_info.h"
#include "services/device_status.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SETUP_ENDPOINT "/api/nfNode/setup-v2"
#define SETUP_COMPLETE_ENDPOINT "/api/nfNode/setup/complete"

static Console csl = {
    .topic = "setup",
    .level = CONSOLE_DEBUG,
};

typedef struct {
    DeviceInfo *device_info;
    char *wayru_device_id;
    AccessToken *access_token;
} SetupTaskContext;

// Backend should handle setup requests that have already been created for this access key
// If no setup request exists, create one
void request_setup(void *task_context) {
    SetupTaskContext *context = (SetupTaskContext *)task_context;
    print_debug(&csl, "Requesting setup");

    // Build setup URL
    char setup_url[256];
    snprintf(setup_url, sizeof(setup_url), "%s%s", config.main_api, SETUP_ENDPOINT);
    print_debug(&csl, "setup_url: %s", setup_url);

    // Request body
    json_object *json_body = json_object_new_object();
    json_object_object_add(json_body, "device_id", json_object_new_string(context->device_info->device_id));
    json_object_object_add(json_body, "mac", json_object_new_string(context->device_info->mac));
    json_object_object_add(json_body, "name", json_object_new_string(context->device_info->name));
    json_object_object_add(json_body, "brand", json_object_new_string(context->device_info->brand));
    json_object_object_add(json_body, "model", json_object_new_string(context->device_info->model));
    json_object_object_add(json_body, "public_ip", json_object_new_string(context->device_info->public_ip));
    json_object_object_add(json_body, "os_name", json_object_new_string(context->device_info->os_name));
    json_object_object_add(json_body, "os_version", json_object_new_string(context->device_info->os_version));
    json_object_object_add(json_body, "os_services_version",
                           json_object_new_string(context->device_info->os_services_version));
    json_object_object_add(json_body, "did_public_key", json_object_new_string(context->device_info->did_public_key));
    json_object_object_add(json_body, "wayru_device_id", json_object_new_string(context->wayru_device_id));

    const char *body = json_object_to_json_string(json_body);
    HttpPostOptions setup_options = {
        .url = setup_url,
        .bearer_token = context->access_token->token,
        .body_json_str = body,
    };

    HttpResult result = http_post(&setup_options);
    json_object_put(json_body);

    if (result.is_error) {
        print_error(&csl, "setup request failed: %s", result.error);
        return;
    }

    if (result.response_buffer == NULL) {
        print_error(&csl, "setup request failed: no response received");
        return;
    }

    print_debug(&csl, "setup request response: %s", result.response_buffer);
}

void setup_task(Scheduler *sch, void *task_context) {
    SetupTaskContext *context = (SetupTaskContext *)task_context;
    if (device_status == Unknown) {
        // Schedule setup_task to rerun later
        // The device's status has to be defined beforehand
        print_debug(&csl, "device status is Unknown, rescheduling setup task");
        schedule_task(sch, time(NULL) + config.setup_interval, setup_task, "setup", context);
    }

    if (device_status == Initial) {
        print_debug(&csl, "requesting setup");
        request_setup(context);
    }
}

void setup_service(Scheduler *sch, DeviceInfo *device_info, char *wayru_device_id, AccessToken *access_token) {
    SetupTaskContext *context = (SetupTaskContext *)malloc(sizeof(SetupTaskContext));
    context->device_info = device_info;
    context->wayru_device_id = wayru_device_id;
    context->access_token = access_token;
    setup_task(sch, context);
}
