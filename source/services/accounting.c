#include "accounting.h"
#include "lib/scheduler.h"
#include "lib/console.h"
#include "lib/requests.h"
#include "lib/script_runner.h"
#include "services/access.h"
#include "services/config.h"
#include "services/device_status.h"
#include "state.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEV_PATH "."
#define OPENWRT_PATH "/etc/wayru-os-services"
#define SCRIPTS_PATH "/scripts"

#define MAX_BUFFER_SIZE 256
#define ACCOUNTING_ENDPOINT "/gateways/connections/accounting"

char command[MAX_BUFFER_SIZE];
char scripts_path[256];

char *query_opennds() {
    console(CONSOLE_DEBUG, "querying OpenNDS");

    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s%s", scripts_path, "/nds-clients.sh");

    char *accounting_output = run_script(script_file);

    // Make sure this is a valid JSON
    struct json_object *parsed_response;
    parsed_response = json_tokener_parse(accounting_output);
    if (parsed_response == NULL) {
        // JSON parsing failed
        console(CONSOLE_ERROR, "failed to parse ndsctl JSON");
        return NULL;
    }

    json_object_put(parsed_response);

    return accounting_output;
}

void deauthenticate_session(const char *client_mac_address) {
    console(CONSOLE_DEBUG, "deauthenticating session %s", client_mac_address);

    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s%s %s", scripts_path, "/nds-deauth.sh", client_mac_address);

    char *deauthenticate_output = run_script(script_file);
    console(CONSOLE_DEBUG, "deauthenticate result -> %s", deauthenticate_output);

    free(deauthenticate_output);
}

size_t process_accounting_response(char *ptr, size_t size, size_t nmemb, void *userdata) {
    console(CONSOLE_DEBUG, "processing accounting response");
    console(CONSOLE_DEBUG, "ptr: %s", ptr);

    size_t realsize = size * nmemb;

    // Parse JSON
    struct json_object *parsed_response;
    struct json_object *end_list;

    // Parse the response as JSON
    parsed_response = json_tokener_parse(ptr);
    if (parsed_response == NULL) {
        // JSON parsing failed
        console(CONSOLE_ERROR, "failed to parse accounting response JSON");
        return realsize;
    }

    // Make sure the 'end_list' key exists,  and extract it
    if (!json_object_object_get_ex(parsed_response, "end_list", &end_list)) {
        console(CONSOLE_ERROR, "'end_list' key not found in JSON");
        json_object_put(parsed_response);
        return realsize;
    }

    // Ensure 'end_list' is an array
    if (!json_object_is_type(end_list, json_type_array)) {
        console(CONSOLE_ERROR, "'end_list' is not an array");
        json_object_put(parsed_response);
        return realsize;
    }

    // Iterate over the end list
    size_t n = json_object_array_length(end_list);
    for (size_t i = 0; i < n; i++) {
        struct json_object *client_mac_address = json_object_array_get_idx(end_list, i);
        deauthenticate_session(json_object_get_string(client_mac_address));
    }

    json_object_put(parsed_response);
    return realsize;
}

void post_accounting_update(char *opennds_clients_data) {
    // Build accounting URL
    char accounting_url[256];
    snprintf(accounting_url, sizeof(accounting_url), "%s%s", config.accounting_api, ACCOUNTING_ENDPOINT);

    console(CONSOLE_DEBUG, "accounting_url: %s", accounting_url);
    console(CONSOLE_DEBUG, "posting accounting update");

    // Request options
    PostRequestOptions post_accounting_options = {
        .url = accounting_url,
        .key = access_key.public_key,
        .body = opennds_clients_data,
        .filePath = NULL,
        .writeFunction = process_accounting_response,
        .writeData = NULL,
    };

    performHttpPost(&post_accounting_options);
}

char *status_opennds() {
    snprintf(command, sizeof(command), "service opennds status");

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        console(CONSOLE_ERROR, "failed to execute ndsctl status command");
        return NULL;
    }

    char *status = (char *)malloc(MAX_BUFFER_SIZE * sizeof(char));
    if (fgets(status, MAX_BUFFER_SIZE, fp) == NULL) {
        console(CONSOLE_ERROR, "failed to read opennds status");
        pclose(fp);
        free(status);
        return NULL;
    }

    pclose(fp);
    return status;
}

int stop_opennds() {
    snprintf(command, sizeof(command), "service opennds stop");

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        console(CONSOLE_ERROR, "failed to execute command opennds stop");
        return -1;
    }

    int status = pclose(fp);
    if (status == -1) {
        console(CONSOLE_ERROR, "failed to close opennds stop command; we could have a memory issue!");
        return -1;
    } else if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status == 0) {
            console(CONSOLE_DEBUG, "openNDS stop command executed successfully");
            return 0;
        } else {
            console(CONSOLE_ERROR, "error executing the opennds stop command; exit code: %d", exit_status);
            return exit_status;
        }
    } else {
        console(CONSOLE_ERROR, "openNDS stop command terminated unexpectedly");
        return -1;
    }
}

int start_opennds() {
    snprintf(command, sizeof(command), "service opennds start");

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        console(CONSOLE_DEBUG, "Error executing command opennds start.");
        return -1;
    }

    int status = pclose(fp);
    if (status == -1) {
        console(CONSOLE_DEBUG, "Error closing opennds start command.");
        return -1;
    } else if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status == 0) {
            console(CONSOLE_DEBUG, "Opennds start command executed successfully.");
            return 0;
        } else {
            console(CONSOLE_DEBUG, "Error executing the opennds start command. Exit code: %d", exit_status);
            return exit_status;
        }
    } else {
        console(CONSOLE_DEBUG, "Opennds start command terminated unexpectedly.");
        return -1;
    }
}

void accounting_task(Scheduler *sch) {
    // Set up paths
    int dev_env = config.dev_env;
    int accounting_enabled = config.accounting_enabled;
    
    if (accounting_enabled == 0) {
        console(CONSOLE_DEBUG, "accounting is disabled by config params (maybe this device doesn't run the captive portal)");
        return;
    }

    if (device_status != Ready) {
        console(CONSOLE_DEBUG, "accounting is disabled by device status");
        return;
    }

    // Set up paths
    char base_path[256];
    if (dev_env == 1) {
        strncpy(base_path, DEV_PATH, sizeof(base_path));
        base_path[sizeof(base_path) - 1] = '\0'; // Ensure null termination
    } else {
        strncpy(base_path, OPENWRT_PATH, sizeof(base_path));
        base_path[sizeof(base_path) - 1] = '\0'; // Ensure null termination
    }

    char *opennds_clients_data = query_opennds();
    if (opennds_clients_data == NULL) {
        console(CONSOLE_DEBUG, "failed to query OpenNDS; skipping server sync");
        return;
    }

    post_accounting_update(opennds_clients_data);

    free(opennds_clients_data);

    schedule_task(&sch, time(NULL) + config.accounting_interval, accounting_task, "accounting");
}

void init_accounting_service(Scheduler *sch) {
    accounting_task(&sch);
}