#include "device-context.h"
#include "lib/console.h"
#include "lib/http-requests.h"
#include "services/access_token.h"
#include "services/config.h"
#include "services/registration.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEVICE_ENDPOINT "devices"
#define DEVICE_CONTEXT_ENDPOINT "context"

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
    DeviceContext device_context;
    device_context.site = NULL;

    json_object *json = json_tokener_parse(device_context_json);
    if (json == NULL) {
        console(CONSOLE_ERROR, "failed to parse device context json");
        return device_context;
    }

    json_object *site_json = NULL;
    if (!json_object_object_get_ex(json, "site", &site_json)) {
        console(CONSOLE_ERROR, "failed to get site from device context json");
        json_object_put(json);
        return device_context;
    }

    device_context.site = strdup(json_object_get_string(site_json));
    json_object_put(json);
    return device_context;
}

DeviceContext *init_device_context(Registration *registration, AccessToken *access_token) {
    DeviceContext *device_context = (DeviceContext *)malloc(sizeof(DeviceContext));
    if (device_context != NULL) {
        device_context->site = NULL;
    }

    char *device_context_json = request_device_context(registration, access_token);
    if (device_context_json == NULL) {
        console(CONSOLE_ERROR, "failed to request device context");
        return device_context;
    }

    DeviceContext parsed_device_context = parse_device_context(device_context_json);
    free(device_context_json);
    device_context->site = parsed_device_context.site;
    console(CONSOLE_DEBUG, "context site %s", device_context->site);
    return device_context;
}
