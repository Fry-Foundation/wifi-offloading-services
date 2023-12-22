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

char *queryOpenNds(char *scriptsPath)
{
    printf("[accounting] querying OpenNDS\n");
    char scriptFile[256];
    snprintf(scriptFile, sizeof(scriptFile), "%s%s", scriptsPath, "/nds-clients.sh");
    char *accountingOutput = runScript(scriptFile);
    return accountingOutput;
}

void postAccountingUpdate(char *scriptsPath)
{
    printf("[accounting] posting accounting update\n");
}

char *deauthenticateSessions(char *scriptsPath)
{
    printf("[accounting] ending sessions\n");
    char scriptFile[256];
    snprintf(scriptFile, sizeof(scriptFile), "%s%s", scriptsPath, "/nds-deauth.sh");
    char *deauthenticateOputput = runScript(scriptFile);
    return deauthenticateOputput;
}

char *statusOpenNds()
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

int stopOpenNds()
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
        int exitStatus = WEXITSTATUS(status);
        if (exitStatus == 0)
        {
            printf("Opennds stop command executed successfully.\n");
            return 0;
        }
        else
        {
            printf("Error executing the opennds stop command. Exit code: %d\n", exitStatus);
            return exitStatus;
        }
    }
    else
    {
        printf("Opennds stop command terminated unexpectedly.\n");
        return -1;
    }
}

int startOpenNds()
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
        int exitStatus = WEXITSTATUS(status);
        if (exitStatus == 0)
        {
            printf("Opennds start command executed successfully.\n");
            return 0;
        }
        else
        {
            printf("Error executing the opennds start command. Exit code: %d\n", exitStatus);
            return exitStatus;
        }
    }
    else
    {
        printf("Opennds start command terminated unexpectedly.\n");
        return -1;
    }
}

void accountingTask(int argc, char *argv[])
{
    // Set up paths
    // int devEnv = 0;
    int devEnv = getConfig().devEnv;

    printf("[init] devEnv: %d\n", devEnv);

    // Set up paths
    char *basePath = (devEnv == 1) ? DEV_PATH : OPENWRT_PATH;
    printf("[init] basePath: %s\n", basePath);

    char scriptsPath[256];
    snprintf(scriptsPath, sizeof(scriptsPath), "%s%s", basePath, "/scripts");

    if (state.accounting != 1)
    {
        printf("[accounting] accounting is disabled\n");
        return;
    }

    printf("[accounting] ccounting task\n");

    // queryOpenNds();
    char *query = queryOpenNds(scriptsPath);
    printf("[accounting] Current clients: %s\n", query);
    // postAccountingUpdate();

    // deauthenticateSessions();
    char *deauth = deauthenticateSessions(scriptsPath);
    printf("[accounting] Deauthenticated clients: %s\n", deauth);
}