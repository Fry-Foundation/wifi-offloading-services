#include "package_update.h"
#include "lib/console.h"
#include "lib/http-requests.h"
#include "lib/result.h"
#include "lib/script_runner.h"
#include "services/access_token.h"
#include "services/config.h"
#include "services/device_info.h"
#include "services/registration.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static Console csl = {
    .topic = "package-update",
};

typedef struct {
    DeviceInfo *device_info;
    Registration *registration;
    AccessToken *access_token;
} PackageUpdateTaskContext;

typedef struct {
    bool update_available;
    const char *download_link;
    const char *checksum;
    const char *size_bytes;
    const char *new_version;
} PackageCheckResult;

#define PACKAGE_STATUS_ENDPOINT "packages/status"
#define PACKAGE_CHECK_ENDPOINT "packages/check"
#define UPDATE_MARKER_FILE "/tmp/wayru-os-services-update-marker"

void send_package_status(PackageUpdateTaskContext *ctx, const char* status, const char* error_message, const char* new_version) {
    // Url
    char package_status_url[256];
    snprintf(package_status_url, sizeof(package_status_url), "%s/%s", config.updates_api, PACKAGE_STATUS_ENDPOINT);

    // Request body (note that error_message is optional)
    json_object *json_body = json_object_new_object();
    json_object_object_add(json_body, "package_name", json_object_new_string("wayru-os-services"));
    json_object_object_add(json_body, "package_architecture", json_object_new_string(ctx->device_info->arch));
    json_object_object_add(json_body, "current_version", json_object_new_string(ctx->device_info->os_services_version));
    json_object_object_add(json_body, "new_version", json_object_new_string(new_version));
    json_object_object_add(json_body, "package_status", json_object_new_string(status));
    json_object_object_add(json_body, "device_id", json_object_new_string(ctx->registration->wayru_device_id));
    if (error_message != NULL) {
        json_object_object_add(json_body, "error_message", json_object_new_string(error_message));
    }

    const char *body = json_object_to_json_string(json_body);

    print_debug(&csl, "package status request body: %s", body);

    // Send status to server
    HttpPostOptions options = {
        .url = package_status_url,
        .body_json_str = body,
        .bearer_token = ctx->access_token->token,
    };

    HttpResult result = http_post(&options);

    json_object_put(json_body);

    if (result.is_error) {
        print_error(&csl, "package status request failed: %s", result.error);

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

    if (result.response_buffer != NULL) {
        free(result.response_buffer);
    }
}

void check_package_update_completion(Registration *registration, DeviceInfo *device_info, AccessToken *access_token) {
    if (access(UPDATE_MARKER_FILE, F_OK) == 0) {
        // Read the file's contents
        char version[64];
        FILE *marker_file = fopen(UPDATE_MARKER_FILE, "r");
        if (marker_file != NULL) {
            fgets(version, sizeof(version), marker_file);
            fclose(marker_file);
        }

        // Compare the version with the current version
        if (strcmp(version, device_info->os_services_version) == 0) {
            print_info(&csl, "Package update completed successfully");
            PackageUpdateTaskContext ctx = {
                .device_info = device_info,
                .registration = registration,
                .access_token = access_token
            };
            send_package_status(&ctx, "completed", NULL, NULL);
        } else {
            print_error(&csl, "Package update failed");
        }

        remove(UPDATE_MARKER_FILE);
    } else {
        print_info(&csl, "No update marker found");
    }
}

void write_update_marker(const char *new_version) {
    FILE *marker_file = fopen(UPDATE_MARKER_FILE, "w");
    if (marker_file != NULL) {
        fprintf(marker_file, "%s", new_version);
        fclose(marker_file);
    }
}

void update_package(const char* file_path) {
    char command[256];
    snprintf(command, sizeof(command), "%s/%s %s", config.scripts_path, "run_opkg_upgrade.sh", file_path);
    char *output = run_script(command);
    if (output != NULL) {
        free(output);
    }
}

Result verify_package_checksum(const char *file_path, const char* checksum) {
    if (file_path == NULL || checksum == NULL) {
        return error(1, "Invalid parameters: file_path or checksum is NULL");
    }

    // Create command to calculate SHA256 checksum
    char command[256];
    snprintf(command, sizeof(command), "sha256sum '%s'", file_path);

    // Run the sha256sum command
    char *output = run_script(command);
    if (output == NULL) {
        return error(2, "Failed to run sha256sum command");
    }

    // Parse the output: sha256sum returns "<hash>  <filename>" (with 2 spaces)
    char calculated_checksum[65]; // SHA256 is 64 characters + null terminator
    sscanf(output, "%64s", calculated_checksum);

    // Free the output buffer
    free(output);

    // Compare the calculated checksum with the expected one
    if (strcmp(calculated_checksum, checksum) == 0) {
        print_debug(&csl, "Checksum verification successful");
        return ok(true);
    } else {
        print_error(&csl, "Checksum mismatch: expected %s, got %s", checksum, calculated_checksum);
        return error(3, "Checksum verification failed");
    }
}


// @todo verify checksum
// @todo report back to log update attempt as "requested"
Result download_package(PackageUpdateTaskContext* ctx, const char* download_link, const char* checksum) {
    if (ctx == NULL || download_link == NULL || checksum == NULL) {
        print_error(&csl, "Invalid parameters");
        return error(-1, "Invalid parameters");
    }

    // Prepare download path
    char download_path[256];
    snprintf(download_path, sizeof(download_path), "%s/%s", config.temp_path, "package-update.ipk");
    print_debug(&csl, "downloading package from: %s to %s", download_link, download_path);

    // Set up download options
    HttpDownloadOptions download_options = {
        .url = download_link,
        .download_path = download_path,
    };

    // Perform the download
    HttpResult download_result = http_download(&download_options);

    if (download_result.is_error) {
        print_error(&csl, "package download failed: %s", download_result.error);
        send_package_status(ctx, "error", "package download failed", NULL);
        return error(-1, "package download failed");
    }

    print_debug(&csl, "package downloaded successfully");
    return ok(strdup(download_path));
}

// Send package check request to the updates API.
Result send_package_check_request(PackageUpdateTaskContext *ctx) {
    // Url
    char package_update_url[256];
    snprintf(package_update_url, sizeof(package_update_url), "%s/%s", config.updates_api, PACKAGE_CHECK_ENDPOINT);
    print_debug(&csl, "package update url: %s", package_update_url);

    // Request body
    json_object *json_body = json_object_new_object();
    json_object_object_add(json_body, "package_name", json_object_new_string("wayru-os-services"));
    json_object_object_add(json_body, "package_architecture", json_object_new_string(ctx->device_info->arch));
    json_object_object_add(json_body, "current_version", json_object_new_string(ctx->device_info->os_services_version));
    json_object_object_add(json_body, "device_id", json_object_new_string(ctx->registration->wayru_device_id));
    const char *body = json_object_to_json_string(json_body);

    print_debug(&csl, "package check request body: %s", body);

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

        return error(-1, "package update request failed");
    }

    if (result.response_buffer == NULL) {
        print_error(&csl, "package update request failed: response buffer is NULL");
        return error(-1, "response buffer is null");
    }

    // Parse server response
    struct json_object *json_parsed_response;
    struct json_object *json_data;
    struct json_object *json_update_available;
    struct json_object *json_download_link;
    struct json_object *json_checksum;
    struct json_object *json_size_bytes;
    struct json_object *json_new_version;

    json_parsed_response = json_tokener_parse(result.response_buffer);
    if (json_parsed_response == NULL) {
        print_error(&csl, "failed to parse package update JSON response");
        free(result.response_buffer);
        return error(-1, "failed to parse package update JSON response");
    }

    // Get the data object, which contains all other fields
    if (!json_object_object_get_ex(json_parsed_response, "data", &json_data)) {
        print_error(&csl, "missing 'data' field in package update response");
        json_object_put(json_parsed_response);
        free(result.response_buffer);
        return error(-1, "missing 'data' field in package update response");
    }

    // Extract fields from the data object
    if (!json_object_object_get_ex(json_data, "update_available", &json_update_available)) {
        print_error(&csl, "missing 'update_available' field in package update response");
        json_object_put(json_parsed_response);
        free(result.response_buffer);
        return error(-1, "missing 'update_available' field in package update response");
    }

    bool update_available = json_object_get_boolean(json_update_available);
    if (!update_available) {
        print_debug(&csl, "no update available");
        json_object_put(json_parsed_response);
        free(result.response_buffer);
        PackageCheckResult check_result = {false, NULL, NULL, NULL, NULL};
        return ok(&check_result);
    }

    bool error_extracting = false;
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
    if (!json_object_object_get_ex(json_data, "new_version", &json_new_version)) {
        print_error(&csl, "missing 'new_version' field in package update response");
        error_extracting = true;
    }

    if (error_extracting) {
        print_error(&csl, "error extracting fields from package update response");
        json_object_put(json_parsed_response);
        free(result.response_buffer);
        return error(-1, "error extracting fields from package update response");
    }


    const char *download_link = json_object_get_string(json_download_link);
    const char *checksum = json_object_get_string(json_checksum);
    const char *size_bytes = json_object_get_string(json_size_bytes);
    const char *new_version = json_object_get_string(json_new_version);

    print_debug(&csl, "download link: %s", download_link);
    print_debug(&csl, "checksum: %s", checksum);
    print_debug(&csl, "size bytes: %s", size_bytes);
    print_debug(&csl, "new version: %s", new_version);

    PackageCheckResult* check_result = malloc(sizeof(PackageCheckResult));
    if (!check_result) {
        return error(-1, "failed to allocate memory for PackageCheckResult");
    }

    check_result->update_available = true;
    check_result->download_link = strdup(download_link);
    check_result->checksum = strdup(checksum);
    check_result->size_bytes = strdup(size_bytes);
    check_result->new_version = strdup(new_version);

    json_object_put(json_parsed_response);
    free(result.response_buffer);

    return ok(check_result);
}

void package_update_task(Scheduler *sch, void *task_context) {
    PackageUpdateTaskContext *ctx = (PackageUpdateTaskContext *)task_context;

    if (config.package_update_enabled == 0) {
        print_debug(&csl, "package update is disabled by configuration; will not reschedule package update task");
        return;
    }

    print_debug(&csl, "package update task");

    // Check if an update is available
    Result result = send_package_check_request(ctx);
    if (!result.ok) {
        print_error(&csl, result.error.message);
        schedule_task(sch, time(NULL) + config.package_update_interval, package_update_task, "package update task", ctx);
        return;
    }

    // Make sure result is valid, and that an update is available
    PackageCheckResult *package_check_result = (PackageCheckResult *)result.data;
    if (!package_check_result) {
        print_error(&csl, "failed to parse package check result");
        schedule_task(sch, time(NULL) + config.package_update_interval, package_update_task, "package update task", ctx);
        return;
    }
    if (!package_check_result->update_available) {
        print_info(&csl, "no package update available");
        schedule_task(sch, time(NULL) + config.package_update_interval, package_update_task, "package update task", ctx);
        return;
    }

    send_package_status(ctx, "in_progress", NULL, package_check_result->new_version);

    // Download the package
    Result download_result = download_package(ctx, package_check_result->download_link, package_check_result->checksum);
    if (!download_result.ok) {
        send_package_status(ctx, "error", download_result.error.message, NULL);
        schedule_task(sch, time(NULL) + config.package_update_interval, package_update_task, "package update task", ctx);
        return;
    }

    // Verify the package's checksum
    const char *download_path = download_result.data;
    Result verify_result = verify_package_checksum(download_path, package_check_result->checksum);
    if (!verify_result.ok) {
        send_package_status(ctx, "error", verify_result.error.message, NULL);
        schedule_task(sch, time(NULL) + config.package_update_interval, package_update_task, "package update task", ctx);
        return;
    }

    // Write the update marker
    write_update_marker(package_check_result->new_version);

    // Proceed with update
    update_package(download_path);

    // Free allocated data
    free((void *)download_path);
    if (package_check_result != NULL) {
        if (package_check_result->download_link != NULL) {
            free((void *)package_check_result->download_link);
        }
        if (package_check_result->checksum != NULL) {
            free((void *)package_check_result->checksum);
        }
        if (package_check_result->size_bytes != NULL) {
            free((void *)package_check_result->size_bytes);
        }
        if (package_check_result->new_version != NULL) {
            free((void *)package_check_result->new_version);
        }
        free((void *)package_check_result);
    }
}

void package_update_service(Scheduler *sch, DeviceInfo *device_info, Registration *registration, AccessToken *access_token) {
    PackageUpdateTaskContext *ctx = (PackageUpdateTaskContext *)malloc(sizeof(PackageUpdateTaskContext));
    if (ctx == NULL) {
        print_error(&csl, "failed to allocate memory for package update task context");
        return;
    }

    ctx->device_info = device_info;
    ctx->registration = registration;
    ctx->access_token = access_token;

    print_debug(&csl, "scheduling package update task");
    package_update_task(sch, ctx);
}
