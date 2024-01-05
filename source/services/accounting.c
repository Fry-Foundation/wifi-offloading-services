#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../store/config.h"
#include "../store/state.h"
#include "accounting.h"
#include "../store/state.h"
#include "../utils/script_runner.h"

#define DEV_PATH "."
#define OPENWRT_PATH "/etc/wayru-os-services"
#define SCRIPTS_PATH "/scripts"

#define MAX_BUFFER_SIZE 256

char command[MAX_BUFFER_SIZE];

char *query_opennds(char *scripts_path)
{
    printf("[accounting] querying OpenNDS\n");
    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s%s", scripts_path, "/nds-clients.sh");
    char *accounting_output = run_script(script_file);
    return accounting_output;
}

void post_accounting_update(char *scripts_path)
{
    printf("[accounting] posting accounting update\n");
}

char *deauthenticate_sessions(char *scripts_path)
{
    printf("[accounting] ending sessions\n");
    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s%s", scripts_path, "/nds-deauth.sh");
    char *deauthenticate_output = run_script(script_file);
    return deauthenticate_output;
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

    printf("[init] dev_env: %d\n", dev_env);

    // Set up paths
    char *base_path = (dev_env == 1) ? DEV_PATH : OPENWRT_PATH;
    printf("[init] base_path: %s\n", base_path);

    char scripts_path[256];
    snprintf(scripts_path, sizeof(scripts_path), "%s%s", base_path, "/scripts");

    if (state.accounting != 1)
    {
        printf("[accounting] accounting is disabled\n");
        return;
    }

    printf("[accounting] accounting task\n");

    // query_opennds();
    char *query = query_opennds(scripts_path);
    printf("[accounting] Current clients: %s\n", query);
    // post_accounting_update();

    // deauthenticate_sessions();
    char *deauth = deauthenticate_sessions(scripts_path);
    printf("[accounting] Deauthenticated clients: %s\n", deauth);

    free(query);
    free(deauth);
}