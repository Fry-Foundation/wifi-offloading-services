#include "firmware_upgrade.h"
#include "lib/console.h"
#include "lib/http-requests.h"
#include "lib/scheduler.h"
#include "lib/script_runner.h"
#include "services/config.h"
#include "services/device_info.h"
#include "services/registration.h"
#include "services/access_token.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FIRMWARE_ENDPOINT "/firmware-updates/check-updates"
#define START_UPGRADE_ENDPOINT "/firmware-updates/start"
#define REPORT_STATUS_ENDPOINT "/firmware-updates/report-status"
#define VERIFY_STATUS_ENDPOINT "/firmware-updates/on-boot"
#define REQUEST_BODY_BUFFER_SIZE 256

typedef struct {
    DeviceInfo *device_info;
    Registration *registration;
    AccessToken *access_token;
} FirmwareUpgradeTaskContext;

int run_sysupgrade() {

    char script_path[256];
    char image_path[256];

    snprintf(script_path, sizeof(script_path), "%s/run_sysupgrade.sh", config.scripts_path);
    snprintf(image_path, sizeof(image_path), "%s", config.temp_path);

    char command[256];
    snprintf(command, sizeof(command), "%s %s", script_path, image_path);
    console(CONSOLE_DEBUG, "Running sysupgrade script: %s", command);

    char *script_output = run_script(command);

    if (script_output) {
        console(CONSOLE_DEBUG, "Sysupgrade script output: %s", script_output);
        int result = (*script_output == '1') ? 1 : -1;
        free(script_output);
        return result;
    }

    return -1;
}

void report_upgrade_status(AccessToken *access_token, int upgrade_attempt_id, const char *upgrade_status) {
    char report_status_url[256];
    snprintf(report_status_url, sizeof(report_status_url), "%s%s", config.accounting_api, REPORT_STATUS_ENDPOINT);
    //snprintf(report_status_url, sizeof(report_status_url), "%s%s", "http://localhost:4050", REPORT_STATUS_ENDPOINT);

    json_object *json_body = json_object_new_object();
    json_object_object_add(json_body, "upgrade_attempt_id", json_object_new_int(upgrade_attempt_id));
    json_object_object_add(json_body, "upgrade_status", json_object_new_string(upgrade_status));
    const char *body = json_object_to_json_string(json_body);

    console(CONSOLE_DEBUG, "Reporting upgrade status with request body: %s", body);

    HttpPostOptions options = {
        .url = report_status_url,
        .body_json_str = body,
        .bearer_token = access_token->token,
    };

    HttpResult result = http_post(&options);

    json_object_put(json_body);

    if (result.is_error) {
        console(CONSOLE_ERROR, "Failed to report upgrade status");
        console(CONSOLE_ERROR, "Error: %s", result.error);
        return;
    }

    if (result.response_buffer == NULL) {
        console(CONSOLE_ERROR, "Failed to report upgrade status");
        console(CONSOLE_ERROR, "No response received");
        return;
    }

    console(CONSOLE_DEBUG, "Reported upgrade status successfully");
    free(result.response_buffer);
}


int execute_firmware_verification() {
    char script_path[256];
    char image_path[256];

    snprintf(script_path, sizeof(script_path), "%s/verify_firmware.sh", config.scripts_path);
    snprintf(image_path, sizeof(image_path), "%s", config.temp_path);

    char command[256];
    snprintf(command, sizeof(command), "%s %s", script_path, image_path);
    console(CONSOLE_DEBUG, "Running command: %s", command);

    char *script_output = run_script(command);

    if (script_output) {
        console(CONSOLE_DEBUG, "Script output: %s", script_output);
        int result = (*script_output == '1') ? 1 : -1;
        free(script_output);
        return result;
    }

    return -1;
}

void handle_download_result(AccessToken *access_token, int upgrade_attempt_id, bool success) {
    if (success) {
        report_upgrade_status(access_token, upgrade_attempt_id, "download_confirmed");

        int script_result = execute_firmware_verification();

        if (script_result == 1) {
            // Verification successful
            console(CONSOLE_INFO, "The image is correct. The hashes match");
            report_upgrade_status(access_token, upgrade_attempt_id, "hash_verification_confirmed");

            int upgrade_result = run_sysupgrade();

            if (upgrade_result == -1) {
                report_upgrade_status(access_token, upgrade_attempt_id, "sysupgrade_failed");
                // schedule_task(NULL, time(NULL) + 3600, firmware_upgrade_task, "sysupgrade_retry");
                // Reschedule task
            }

            /*if (upgrade_result == 1) {
                report_upgrade_status(upgrade_attempt_id, "upgrading");
            } else {
                report_upgrade_status(upgrade_attempt_id, "sysupgrade_failed");
                // Reschedule task
                // schedule_task(NULL, time(NULL) + 3600, firmware_upgrade_task, "sysupgrade_retry");
            }*/

        } else {
            // Verification failed
            console(CONSOLE_INFO, "TThe image is incorrect. The hashes do not match");
            report_upgrade_status(access_token, upgrade_attempt_id, "hash_verification_failed");
            // Reschedule verification in an hour
            // schedule_task(NULL, time(NULL) + 3600, firmware_upgrade_task, "hash_verification_retry");
        }

    } else {
        report_upgrade_status(access_token, upgrade_attempt_id, "download_failed");
        // Schedule a retry in one hour
        // schedule_task(sch, time(NULL) + 3600, firmware_upgrade_task, "firmware_upgrade_retry");
    }
}

void send_firmware_check_request(const char *codename, const char *version, const char *wayru_device_id, AccessToken *access_token) {

    if (config.firmware_update_enabled == 0) {
        console(CONSOLE_DEBUG, "Firmware update is disabled by configuration; will not proceed");
        return;
    }

    // Url
    char firmware_upgrade_url[256];
    snprintf(firmware_upgrade_url, sizeof(firmware_upgrade_url), "%s%s", config.accounting_api, FIRMWARE_ENDPOINT);
    //snprintf(firmware_upgrade_url, sizeof(firmware_upgrade_url), "%s%s", "http://localhost:4050", FIRMWARE_ENDPOINT);

    console(CONSOLE_DEBUG, "Firmware endpoint: %s", firmware_upgrade_url);

    // Request body
    json_object *json_body = json_object_new_object();
    json_object_object_add(json_body, "codename", json_object_new_string(codename));
    json_object_object_add(json_body, "version", json_object_new_string(version));
    json_object_object_add(json_body, "wayru_device_id", json_object_new_string(wayru_device_id));
    const char *body = json_object_to_json_string(json_body);

    console(CONSOLE_DEBUG, "Check firmware update body %s", body);

    HttpPostOptions options = {
        .url = firmware_upgrade_url,
        .body_json_str = body,
        .bearer_token = access_token->token,
    };

    HttpResult result = http_post(&options);

    json_object_put(json_body);

    if (result.is_error) {
        console(CONSOLE_ERROR, "Failed to check firmware update");
        console(CONSOLE_ERROR, "Error: %s", result.error);
        return;
    }

    if (result.response_buffer == NULL) {
        console(CONSOLE_ERROR, "Failed to check firmware update");
        console(CONSOLE_ERROR, "No response received");
        return;
    }

    // Parse server response
    struct json_object *parsed_response;
    struct json_object *updateAvailable;
    struct json_object *url = NULL;
    struct json_object *latestVersion;
    struct json_object *id = NULL;

    parsed_response = json_tokener_parse(result.response_buffer);
    if (parsed_response == NULL) {
        // JSON parsing failed
        console(CONSOLE_ERROR, "Failed to parse firmware update JSON data");
        free(result.response_buffer);
        return;
    }

    // Extract fields
    bool error_occurred = false;
    if (!json_object_object_get_ex(parsed_response, "updateAvailable", &updateAvailable)) {
        console(CONSOLE_ERROR, "updateAvailable field missing or invalid");
        error_occurred = true;
    }

    if (!json_object_object_get_ex(parsed_response, "url", &url)) {
        console(CONSOLE_ERROR, "URL field missing or invalid, setting to default");
        url = NULL;
    }

    if (!json_object_object_get_ex(parsed_response, "latestVersion", &latestVersion)) {
        console(CONSOLE_ERROR, "latestVersion field missing or invalid");
        error_occurred = true;
    }

    if (!json_object_object_get_ex(parsed_response, "id", &id)) {
        console(CONSOLE_ERROR, "id field missing or invalid,setting to default");
        id = NULL;
    }

    if (error_occurred) {
        console(CONSOLE_ERROR, "Error processing firmware update response");
        json_object_put(parsed_response);
        free(result.response_buffer);
        return;
    }

    int update_available = json_object_get_int(updateAvailable);
    const char *update_url = (url != NULL) ? json_object_get_string(url) : NULL;
    const char *latest_version = json_object_get_string(latestVersion);
    int upgrade_attempt_id = (id != NULL) ? json_object_get_int(id) : -1;

    console(CONSOLE_DEBUG, "Firmware update available: %d", update_available);
    console(CONSOLE_DEBUG, "Firmware update message or URL: %s", update_url);
    console(CONSOLE_DEBUG, "Target firmware version: %s", latest_version);

    if (update_available == 2) {

        console(CONSOLE_DEBUG, "Starting firmware download from: %s", update_url);

        char download_path[256];
        snprintf(download_path, sizeof(download_path), "%s/firmware.tar.gz", config.temp_path);

        // Download firmware
        HttpDownloadOptions download_options = {
            .url = update_url,
            .download_path = download_path,
        };

        HttpResult download_result = http_download(&download_options);
        handle_download_result(access_token, upgrade_attempt_id, !download_result.is_error);

    } else if (update_available == 1) {
        console(CONSOLE_DEBUG, "New version available: %s. Update pending.", latest_version);
        // Retask
    } else if (update_available == 0) {
        console(CONSOLE_INFO, "No firmware updates available.");
    } else {
        console(CONSOLE_ERROR, "Unknown updateAvailable value received: %d", update_available);
    }

    json_object_put(parsed_response);
    free(result.response_buffer);
}

void firmware_upgrade_task(Scheduler *sch, void *task_context) {
    FirmwareUpgradeTaskContext *context = (FirmwareUpgradeTaskContext *)task_context;

    if (config.firmware_update_enabled == 0) {
        console(CONSOLE_DEBUG, "Firmware update is disabled by configuration; will not reschedule reboot task");
        return;
    }

    console(CONSOLE_DEBUG, "Firmware upgrade task");
    send_firmware_check_request(context->device_info->name, context->device_info->os_version,
                                context->registration->wayru_device_id, context->access_token);
    schedule_task(sch, time(NULL) + config.firmware_update_interval, firmware_upgrade_task, "firmware_upgrade", context);
}

void firmware_upgrade_check(Scheduler *scheduler, DeviceInfo *device_info, Registration *registration, AccessToken *access_token) {
    FirmwareUpgradeTaskContext *context = (FirmwareUpgradeTaskContext *)malloc(sizeof(FirmwareUpgradeTaskContext));
    if (context == NULL) {
        console(CONSOLE_ERROR, "Failed to allocate memory for firmware upgrade task context");
        return;
    }

    context->device_info = device_info;
    context->registration = registration;
    context->access_token = access_token;

    console(CONSOLE_DEBUG, "scheduling firmware upgrade check");
    firmware_upgrade_task(scheduler, context);
}

void clean_firmware_upgrade_service() {
    // Clean up if necessary
}

void firmware_upgrade_on_boot(Registration *registration, DeviceInfo *device_info, AccessToken *access_token) {

    if (!config.firmware_update_enabled) {
        console(CONSOLE_DEBUG, "Firmware upgrade on boot is disabled by configuration; will not proceed.");
        return;
    }

    console(CONSOLE_DEBUG, "Starting firmware_upgrade_on_boot");
    char verify_status_url[256];
    snprintf(verify_status_url, sizeof(verify_status_url), "%s%s", config.accounting_api, VERIFY_STATUS_ENDPOINT);
    //snprintf(verify_status_url, sizeof(verify_status_url), "%s%s", "http://localhost:4050", VERIFY_STATUS_ENDPOINT);

    if (registration == NULL || registration->wayru_device_id == NULL) {
        console(CONSOLE_ERROR, "Registration or wayru_device_id is NULL");
        return;
    }

    if (device_info == NULL || device_info->os_version == NULL) {
        console(CONSOLE_ERROR, "DeviceInfo or os_version is NULL");
        return;
    }

    json_object *json_body = json_object_new_object();
    json_object_object_add(json_body, "wayru_device_id", json_object_new_string(registration->wayru_device_id));
    json_object_object_add(json_body, "os_version", json_object_new_string(device_info->os_version));
    const char *body = json_object_to_json_string(json_body);

    console(CONSOLE_DEBUG, "Verifying firmware status on boot with request body: %s", body);

    HttpPostOptions options = {
        .url = verify_status_url,
        .body_json_str = body,
        .bearer_token = access_token->token,
    };

    HttpResult result = http_post(&options);
    console(CONSOLE_DEBUG, "HTTP request completed");

    json_object_put(json_body);

    if (result.is_error) {
        console(CONSOLE_ERROR, "Failed to verify firmware status on boot");
        console(CONSOLE_ERROR, "Error: %s", result.error);
        return;
    }

    if (result.response_buffer == NULL) {
        console(CONSOLE_ERROR, "Failed to verify firmware status on boot");
        console(CONSOLE_ERROR, "No response received");
        return;
    }

    // Process the server's response
    struct json_object *parsed_response;
    struct json_object *status;

    parsed_response = json_tokener_parse(result.response_buffer);
    if (parsed_response == NULL) {
        console(CONSOLE_ERROR, "Failed to parse verification response JSON data");
        free(result.response_buffer);
        return;
    }

    if (!json_object_object_get_ex(parsed_response, "status", &status)) {
        console(CONSOLE_ERROR, "Status field missing or invalid");
        json_object_put(parsed_response);
        free(result.response_buffer);
        return;
    }

    const char *status_value = json_object_get_string(status);
    console(CONSOLE_DEBUG, "Firmware status on boot: %s", status_value);

    json_object_put(parsed_response);
    free(result.response_buffer);
}
