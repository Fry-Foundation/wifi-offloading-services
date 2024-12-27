#include "accounting.h"
#include "lib/console.h"
#include "lib/http-requests.h"
#include "lib/scheduler.h"
#include "lib/script_runner.h"
#include "services/config.h"
#include "services/device_status.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEV_PATH "."
#define OPENWRT_PATH "/etc/wayru-os-services"
#define SCRIPTS_PATH "/scripts"

#define MAX_BUFFER_SIZE 256
#define ACCOUNTING_ENDPOINT "/gateways/connections/accounting"

static Console csl = {
    .topic = "accounting",
};

char command[MAX_BUFFER_SIZE];
char scripts_path[256];

char *query_opennds() {
    print_debug(&csl, "querying OpenNDS");

    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s%s", scripts_path, "/nds-clients.sh");

    char *accounting_output = run_script(script_file);

    // Make sure this is a valid JSON
    struct json_object *parsed_response;
    parsed_response = json_tokener_parse(accounting_output);
    if (parsed_response == NULL) {
        // JSON parsing failed
        print_error(&csl, "failed to parse ndsctl JSON");
        return NULL;
    }

    json_object_put(parsed_response);

    return accounting_output;
}

void deauthenticate_session(const char *client_mac_address) {
    print_debug(&csl, "deauthenticating session %s", client_mac_address);

    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s%s %s", scripts_path, "/nds-deauth.sh", client_mac_address);

    char *deauthenticate_output = run_script(script_file);
    print_debug(&csl, "deauthenticate result -> %s", deauthenticate_output);

    free(deauthenticate_output);
}

void post_accounting_update(char *opennds_clients_data) {
    // Build accounting URL
    char accounting_url[256];
    snprintf(accounting_url, sizeof(accounting_url), "%s%s", config.accounting_api, ACCOUNTING_ENDPOINT);

    print_debug(&csl, "accounting_url: %s", accounting_url);
    print_debug(&csl, "posting accounting update");

    // Request options
    HttpPostOptions post_accounting_options = {
        .url = accounting_url,
        .body_json_str = opennds_clients_data,
    };

    HttpResult result = http_post(&post_accounting_options);
    if (result.is_error) {
        print_error(&csl, "failed to post accounting update");
        print_error(&csl, "error: %s", result.error);
        return;
    }

    if (result.response_buffer == NULL) {
        print_error(&csl, "failed to post accounting update");
        print_error(&csl, "no response received");
        return;
    }

    // Parse the response as JSON
    struct json_object *parsed_response;
    struct json_object *end_list;

    parsed_response = json_tokener_parse(result.response_buffer);
    if (parsed_response == NULL) {
        // JSON parsing failed
        print_error(&csl, "failed to parse accounting response JSON");
        return;
    }

    // Make sure the 'end_list' key exists,  and extract it
    if (!json_object_object_get_ex(parsed_response, "end_list", &end_list)) {
        print_error(&csl, "'end_list' key not found in JSON");
        json_object_put(parsed_response);
        return;
    }

    // Ensure 'end_list' is an array
    if (!json_object_is_type(end_list, json_type_array)) {
        print_error(&csl, "'end_list' is not an array");
        json_object_put(parsed_response);
        return;
    }

    // Iterate over the end list
    size_t n = json_object_array_length(end_list);
    for (size_t i = 0; i < n; i++) {
        struct json_object *client_mac_address = json_object_array_get_idx(end_list, i);
        deauthenticate_session(json_object_get_string(client_mac_address));
    }

    json_object_put(parsed_response);
    free(result.response_buffer);
}

char *status_opennds() {
    snprintf(command, sizeof(command), "service opennds status");

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        print_error(&csl, "failed to execute ndsctl status command");
        return NULL;
    }

    char *status = (char *)malloc(MAX_BUFFER_SIZE * sizeof(char));
    if (fgets(status, MAX_BUFFER_SIZE, fp) == NULL) {
        print_error(&csl, "failed to read opennds status");
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
        print_error(&csl, "failed to execute command opennds stop");
        return -1;
    }

    int status = pclose(fp);
    if (status == -1) {
        print_error(&csl, "failed to close opennds stop command");
        return -1;
    } else if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status == 0) {
            print_debug(&csl, "openNDS stop command executed successfully");
            return 0;
        } else {
            print_error(&csl, "error executing the opennds stop command; exit code: %d", exit_status);
            return exit_status;
        }
    } else {
        print_error(&csl, "openNDS stop command terminated unexpectedly");
        return -1;
    }
}

int start_opennds() {
    snprintf(command, sizeof(command), "service opennds start");

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        print_debug(&csl, "failed to execute command opennds start");
        return -1;
    }

    int status = pclose(fp);
    if (status == -1) {
        print_debug(&csl, "failed to close opennds start command");
        return -1;
    } else if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status == 0) {
            print_debug(&csl, "opennds start command executed successfully");
            return 0;
        } else {
            print_debug(&csl, "error executing the opennds start command; exit code: %d", exit_status);
            return exit_status;
        }
    } else {
        print_debug(&csl, "opennds start command terminated unexpectedly");
        return -1;
    }
}

void accounting_task(Scheduler *sch, void *task_context) {
    (void)task_context;

    // Set up paths
    int dev_env = config.dev_env;
    int accounting_enabled = config.accounting_enabled;

    if (accounting_enabled == 0) {
        print_debug(&csl, "accounting is disabled by config params");
        print_debug(&csl, "maybe this device doesn't run the captive portal");
        print_debug(&csl, "will not reschedule accounting task");
        return;
    }

    if (device_status != Ready) {
        print_debug(&csl, "device is not ready; will try again later");
        schedule_task(sch, time(NULL) + config.accounting_interval, accounting_task, "accounting", NULL);
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
        print_debug(&csl, "failed to query opennds; skipping server sync, will try again later");
        schedule_task(sch, time(NULL) + config.accounting_interval, accounting_task, "accounting", NULL);
        return;
    }

    post_accounting_update(opennds_clients_data);

    free(opennds_clients_data);

    schedule_task(sch, time(NULL) + config.accounting_interval, accounting_task, "accounting", NULL);
}

void accounting_service(Scheduler *sch) { 
        accounting_task(sch, NULL);
}
