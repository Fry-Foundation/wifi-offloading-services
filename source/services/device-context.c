#include "device-context.h"
#include "lib/console.h"
#include "lib/http-requests.h"
#include "lib/scheduler.h"
#include "services/access_token.h"
#include "services/config.h"
#include "services/registration.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEVICE_ENDPOINT "devices"
#define DEVICE_CONTEXT_ENDPOINT "context"

typedef struct {
    DeviceContext *device_context;
    Registration *registration;
    AccessToken *access_token;
} DeviceContextTaskContext;

char *request_device_context(Registration *registration, AccessToken *access_token) {
    char url[256];
    snprintf(url, sizeof(url), "%s/%s/%s/%s", config.accounting_api, DEVICE_ENDPOINT, registration->wayru_device_id,
             DEVICE_CONTEXT_ENDPOINT);

    HttpGetOptions options = {.url = url, .bearer_token = access_token->token};

    HttpResult result = http_get(&options);
    if (result.is_error) {
        console(CONSOLE_ERROR, "failed to request device context");
        console(CONSOLE_ERROR, "error: %s", result.error);
        return NULL;
    }

    if (result.response_buffer == NULL) {
        console(CONSOLE_ERROR, "failed to request device context");
        return NULL;
    }

    return result.response_buffer;
}

DeviceContext parse_device_context(const char *device_context_json) {
    DeviceContext _device_context;
    _device_context.site = NULL;

    json_object *json = json_tokener_parse(device_context_json);
    if (json == NULL) {
        console(CONSOLE_ERROR, "failed to parse device context json");
        return _device_context;
    }

    json_object *site_json = NULL;
    if (!json_object_object_get_ex(json, "site", &site_json)) {
        console(CONSOLE_ERROR, "failed to get site from device context json");
        json_object_put(json);
        return _device_context;
    }

    _device_context.site = strdup(json_object_get_string(site_json));
    json_object_put(json);
    return _device_context;
}

DeviceContext *init_device_context(Registration *registration, AccessToken *access_token) {
    DeviceContext *_device_context = (DeviceContext *)malloc(sizeof(DeviceContext));
    if (_device_context != NULL) {
        _device_context->site = NULL;
    }

    char *device_context_json = request_device_context(registration, access_token);
    if (device_context_json == NULL) {
        console(CONSOLE_ERROR, "failed to request device context");
        return _device_context;
    }

    DeviceContext parsed_device_context = parse_device_context(device_context_json);
    free(device_context_json);
    _device_context->site = parsed_device_context.site;
    console(CONSOLE_DEBUG, "context site %s", _device_context->site);
    return _device_context;
}

void device_context_task(Scheduler *sch, void *task_context) {
    DeviceContextTaskContext *context = (DeviceContextTaskContext *)task_context;

    char *device_context_json = request_device_context(context->registration, context->access_token);
    if (device_context_json == NULL) {
        console(CONSOLE_ERROR, "failed to request device context");
        return;
    }

    DeviceContext parsed_device_context = parse_device_context(device_context_json);
    free(device_context_json);
    context->device_context->site = parsed_device_context.site;
    console(CONSOLE_DEBUG, "device context site %s", context->device_context->site);
    schedule_task(sch, time(NULL) + config.device_status_interval, device_context_task, "device context", context);
}

void device_context_service(Scheduler *sch,
                            DeviceContext *device_context,
                            Registration *registration,
                            AccessToken *access_token) {
    DeviceContextTaskContext *context = (DeviceContextTaskContext *)malloc(sizeof(DeviceContextTaskContext));
    if (context == NULL) {
        console(CONSOLE_ERROR, "failed to allocate memory for access token task context");
        return;
    }

    context->device_context = device_context;
    context->registration = registration;
    context->access_token = access_token;

    device_context_task(sch, context);
}

void clean_device_context(DeviceContext *device_context) {
    if (device_context->site != NULL) {
        free(device_context->site);
    }

    free(device_context);
}
