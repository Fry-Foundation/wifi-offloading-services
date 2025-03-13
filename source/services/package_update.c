#include "package_update.h"
#include "lib/console.h"
#include "lib/http-requests.h"
#include "services/access_token.h"
#include "services/config.h"
#include "services/device_info.h"
#include "services/registration.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>

static Console csl = {
    .topic = "package-update",
};

typedef struct {
    DeviceInfo *device_info;
    Registration *registration;
    AccessToken *access_token;
} PackageUpdateTaskContext;

#define PACKAGE_UPDATE_CHECK_ENDPOINT "packages/check"

void send_package_check_request(PackageUpdateTaskContext *ctx) {
    // Url
    char package_update_url[256];
    snprintf(package_update_url, sizeof(package_update_url), "%s/%s", config.updates_api, PACKAGE_UPDATE_CHECK_ENDPOINT);
    print_debug(&csl, "package update url: %s", package_update_url);

    // Request body
    json_object *json_body = json_object_new_object();
    json_object_object_add(json_body, "package_name", json_object_new_string("wayru-os-services"));
    json_object_object_add(json_body, "package_architecture", json_object_new_string(ctx->device_info->arch));
    json_object_object_add(json_body, "current_version", json_object_new_string(ctx->device_info->os_services_version));
    json_object_object_add(json_body, "device_id", json_object_new_string(ctx->registration->wayru_device_id));
    const char *body = json_object_to_json_string(json_body);

    print_debug(&csl, "package update request body: %s", body);

    HttpPostOptions options = {
        .url = package_update_url,
        .body_json_str = body,
        .bearer_token = ctx->access_token->token,
    };

    HttpResult result = http_post(&options);

    json_object_put(json_body);

    if (result.is_error) {
        print_error(&csl, "package update request failed: %s", result.error);

        // Try to parse response buffer to get error.message
        if (result.response_buffer != NULL) {
            struct json_object *json_parsed_error = json_tokener_parse(result.response_buffer);
            if (json_parsed_error != NULL) {
                struct json_object *json_error;
                if (json_object_object_get_ex(json_parsed_error, "error", &json_error)) {
                    struct json_object *json_message;
                    if (json_object_object_get_ex(json_error, "message", &json_message)) {
                        const char *error_message = json_object_get_string(json_message);
                        print_error(&csl, "error message from server: %s", error_message);
                    }
                }
                json_object_put(json_parsed_error);
            }
            free(result.response_buffer);
        }

        return;
    }

    if (result.response_buffer == NULL) {
        print_error(&csl, "package update request failed: response buffer is NULL");
        return;
    }

    // Parse server response
    struct json_object *json_parsed_response;
    struct json_object *json_data;
    struct json_object *json_update_available;
    struct json_object *json_download_link;
    struct json_object *json_checksum;
    struct json_object *json_size_bytes;

    json_parsed_response = json_tokener_parse(result.response_buffer);
    if (json_parsed_response == NULL) {
        print_error(&csl, "failed to parse package update JSON response");
        free(result.response_buffer);
        return;
    }

    // Get the data object, which contains all other fields
    if (!json_object_object_get_ex(json_parsed_response, "data", &json_data)) {
        print_error(&csl, "missing 'data' field in package update response");
        json_object_put(json_parsed_response);
        free(result.response_buffer);
        return;
    }

    // Extract fields from the data object
    bool error_extracting = false;
    if (!json_object_object_get_ex(json_data, "update_available", &json_update_available)) {
        print_error(&csl, "missing 'update_available' field in package update response");
        error_extracting = true;
    }
    if (!json_object_object_get_ex(json_data, "download_link", &json_download_link)) {
        print_error(&csl, "missing 'download_link' field in package update response");
        error_extracting = true;
    }
    if (!json_object_object_get_ex(json_data, "checksum", &json_checksum)) {
        print_error(&csl, "missing 'checksum' field in package update response");
        error_extracting = true;
    }
    if (!json_object_object_get_ex(json_data, "size_bytes", &json_size_bytes)) {
        print_error(&csl, "missing 'size_bytes' field in package update response");
        error_extracting = true;
    }

    if (error_extracting) {
        print_error(&csl, "error extracting fields from package update response");
        json_object_put(json_parsed_response);
        free(result.response_buffer);
        return;
    }

    bool update_available = json_object_get_boolean(json_update_available);
    if (!update_available) {
        print_debug(&csl, "no update available");
        return;
    }

    const char *download_link = json_object_get_string(json_download_link);
    const char *checksum = json_object_get_string(json_checksum);
    const char *size_bytes = json_object_get_string(json_size_bytes);

    // @TODO: Implement package download and update logic

    print_debug(&csl, "download link: %s", download_link);
    print_debug(&csl, "checksum: %s", checksum);
    print_debug(&csl, "size bytes: %s", size_bytes);

    json_object_put(json_parsed_response);
    free(result.response_buffer);
}

void package_update_task(Scheduler *sch, void *task_context) {
    PackageUpdateTaskContext *ctx = (PackageUpdateTaskContext *)task_context;

    if (config.package_update_enabled == 0) {
        print_debug(&csl, "package update is disabled by configuration; will not reschedule package update task");
        return;
    }

    print_debug(&csl, "package update task");
    send_package_check_request(ctx);
    schedule_task(sch, time(NULL) + config.package_update_interval, package_update_task, "package update task", ctx);
}

void package_update_service(Scheduler *sch, DeviceInfo *device_info, Registration *registration, AccessToken *access_token) {
    PackageUpdateTaskContext *context = (PackageUpdateTaskContext *)malloc(sizeof(PackageUpdateTaskContext));
    if (context == NULL) {
        print_error(&csl, "failed to allocate memory for package update task context");
        return;
    }

    context->device_info = device_info;
    context->registration = registration;
    context->access_token = access_token;

    print_debug(&csl, "scheduling package update task");
    package_update_task(sch, context);
}
