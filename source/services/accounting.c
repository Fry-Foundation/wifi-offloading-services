#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <json-c/json.h>
#include "../store/config.h"
#include "../store/state.h"
#include "accounting.h"
#include "../utils/requests.h"
#include "../utils/script_runner.h"

#define DEV_PATH "."
#define OPENWRT_PATH "/etc/wayru-os-services"
#define SCRIPTS_PATH "/scripts"

#define MAX_BUFFER_SIZE 256
#define ACCOUNTING_ENDPOINT "/gateways/connections/accounting/"

char command[MAX_BUFFER_SIZE];
char scripts_path[256];

char *query_opennds()
{
    printf("[accounting] querying OpenNDS\n");

    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s%s", scripts_path, "/nds-clients.sh");

    char *accounting_output = run_script(script_file);
    printf("[accounting] query output: %s", accounting_output);

    // Make sure this is a valid JSON
    struct json_object *parsed_response;
    parsed_response = json_tokener_parse(accounting_output);
    if (parsed_response == NULL)
    {
        // JSON parsing failed
        fprintf(stderr, "[accounting] failed to parse ndsctl JSON\n");
        return NULL;
    }

    json_object_put(parsed_response);

    return accounting_output;
}

void deauthenticate_session(const char *client_mac_address)
{
    printf("[accounting] ending session %s\n", client_mac_address);

    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s%s %s", scripts_path, "/nds-deauth.sh", client_mac_address);
    
    char *deauthenticate_output = run_script(script_file);
    printf("[accounting] deauthenticate: %s\n", deauthenticate_output);

    free(deauthenticate_output);
}

size_t process_accounting_response(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    printf("[accounting] processing accounting response\n");
    printf("[accounting] ptr: %s\n", ptr);

    size_t realsize = size * nmemb;

    // Parse JSON
    struct json_object *parsed_response;
    struct json_object *end_list;

    // Parse the response as JSON
    parsed_response = json_tokener_parse(ptr);
    if (parsed_response == NULL)
    {
        // JSON parsing failed
        fprintf(stderr, "[accounting] failed to parse accounting response JSON\n");
        return realsize;
    }

    // Make sure the 'end_list' key exists,  and extract it
    if (!json_object_object_get_ex(parsed_response, "end_list", &end_list)) {
        fprintf(stderr, "[accounting] 'end_list' key not found in JSON\n");
        json_object_put(parsed_response);
        return realsize;
    }

    // Ensure 'end_list' is an array
    if (!json_object_is_type(end_list, json_type_array)) {
        fprintf(stderr, "[accounting] 'end_list' is not an array\n");
        json_object_put(parsed_response);
        return realsize;
    }

    // Iterate over the end list
    size_t n = json_object_array_length(end_list);
    for (size_t i = 0; i < n; i ++) {
        struct json_object *client_mac_address = json_object_array_get_idx(end_list, i);
        deauthenticate_session(json_object_get_string(client_mac_address));
    }

    json_object_put(parsed_response);
    return realsize;
}

void post_accounting_update(char *opennds_clients_data)
{
    printf("[accounting] posting accounting update\n");

    // Build accounting URL
    char accounting_url[256];
    snprintf(accounting_url, sizeof(accounting_url), "%s%s", getConfig().accounting_api, ACCOUNTING_ENDPOINT);
    printf("[accounting] accounting_url: %s\n", accounting_url);

    // Request options
    PostRequestOptions post_accounting_options = {
        .url = accounting_url,
        .key = state.accessKey->key,
        .body = opennds_clients_data,
        .filePath = NULL,
        .writeFunction = process_accounting_response,
        .writeData = NULL,
    };

    performHttpPost(&post_accounting_options);
}

char *status_opennds()
{
    snprintf(command, sizeof(command), "service opennds status");

    FILE *fp = popen(command, "r");
    if (fp == NULL)
    {
        printf("Error executing command, opennds status.\n");
        return NULL;
    }

    char *status = (char *)malloc(MAX_BUFFER_SIZE * sizeof(char));
    if (fgets(status, MAX_BUFFER_SIZE, fp) == NULL)
    {
        printf("Could not read command opennds status.\n");
        pclose(fp);
        free(status);
        return NULL;
    }

    pclose(fp);
    return status;
}

int stop_opennds()
{
    snprintf(command, sizeof(command), "service opennds stop");

    FILE *fp = popen(command, "r");
    if (fp == NULL)
    {
        printf("Error executing command opennds stop.\n");
        return -1;
    }

    int status = pclose(fp);
    if (status == -1)
    {
        printf("Error closing opennds stop command.\n");
        return -1;
    }
    else if (WIFEXITED(status))
    {
        int exit_status = WEXITSTATUS(status);
        if (exit_status == 0)
        {
            printf("Opennds stop command executed successfully.\n");
            return 0;
        }
        else
        {
            printf("Error executing the opennds stop command. Exit code: %d\n", exit_status);
            return exit_status;
        }
    }
    else
    {
        printf("Opennds stop command terminated unexpectedly.\n");
        return -1;
    }
}

int start_opennds()
{
    snprintf(command, sizeof(command), "service opennds start");

    FILE *fp = popen(command, "r");
    if (fp == NULL)
    {
        printf("Error executing command opennds start.\n");
        return -1;
    }

    int status = pclose(fp);
    if (status == -1)
    {
        printf("Error closing opennds start command.\n");
        return -1;
    }
    else if (WIFEXITED(status))
    {
        int exit_status = WEXITSTATUS(status);
        if (exit_status == 0)
        {
            printf("Opennds start command executed successfully.\n");
            return 0;
        }
        else
        {
            printf("Error executing the opennds start command. Exit code: %d\n", exit_status);
            return exit_status;
        }
    }
    else
    {
        printf("Opennds start command terminated unexpectedly.\n");
        return -1;
    }
}

void accounting_task(int argc, char *argv[])
{
    // Set up paths
    int dev_env = getConfig().devEnv;

    printf("[accounting] dev_env: %d\n", dev_env);

    // Set up paths
    char base_path[256];
    if (dev_env == 1) {
        strncpy(base_path, DEV_PATH, sizeof(base_path));
        base_path[sizeof(base_path) - 1] = '\0'; // Ensure null termination
    } else {
        strncpy(base_path, OPENWRT_PATH, sizeof(base_path));
        base_path[sizeof(base_path) - 1] = '\0'; // Ensure null termination
    }
    snprintf(scripts_path, sizeof(scripts_path), "%s%s", base_path, "/scripts");
    printf("[accounting] scripts_path: %s\n", scripts_path);    

    if (state.accounting != 1)
    {
        printf("[accounting] accounting is disabled\n");
        return;
    }

    printf("[accounting] accounting task\n");

    char *opennds_clients_data = query_opennds();
    if (opennds_clients_data == NULL)
    {
        printf("[accounting] failed to query OpenNDS; skipping server sync\n");
        return;
    }

    printf("[accounting] current clients: %s\n", opennds_clients_data);
    post_accounting_update(opennds_clients_data);

    free(opennds_clients_data);
}