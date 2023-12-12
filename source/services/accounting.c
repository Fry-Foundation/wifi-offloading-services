#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../store/config.h"
#include "../store/state.h"
#include "accounting.h"
#include "../store/state.h"
#include "../utils/script_runner.h"

#define DEV_PATH "."
#define OPENWRT_PATH "/etc/wayru"
#define SCRIPTS_PATH "/scripts"

#define MAX_BUFFER_SIZE 1024

char *executeCommand(const char *command)
{
    FILE *fp;
    char *output = (char *)malloc(sizeof(char) * MAX_BUFFER_SIZE);

    if (output == NULL)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }

    // Ejecuta el comando pasado como argumento y captura su salida
    fp = popen(command, "r");

    if (fp == NULL)
    {
        fprintf(stderr, "Failed to execute command.\n");
        free(output);
        return NULL;
    }

    // Lee la salida del comando
    if (fgets(output, MAX_BUFFER_SIZE, fp) == NULL)
    {
        fprintf(stderr, "Failed to read command output.\n");
    }

    // Cierra el pipe
    pclose(fp);

    return output;
}

char *queryOpenNds(char *scriptsPath, int devEnv)
{
    printf("[accounting] querying OpenNDS\n");
    char scriptFile[256];
    if (devEnv != 1)
    {
        // return executeCommand(command);
        snprintf(scriptFile, sizeof(scriptFile), "%s%s", scriptsPath, "/nds-clients.sh");
        char *accountingOputput = runScript(scriptFile);
    }
    else
    {
        snprintf(scriptFile, sizeof(scriptFile), "%s%s", scriptsPath, "/nds-clients.sh");
        char *accountingOputput = runScript(scriptFile);
    }
}

void postAccountingUpdate()
{
    printf("[accounting] posting accounting update\n");
}

void deauthenticateSessions()
{
    printf("[accounting] ending sessions\n");
}

void accountingTask(int argc, char *argv[])
{
    // Set up paths
    int devEnv = 0;
    // const char *command = "ndsctl json";

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--dev") == 0)
        {
            devEnv = 1;
            break;
        }
    }
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
    char *query = queryOpenNds(scriptsPath, devEnv);
    printf("[accounting] Current clients: %s\n", query);
    // postAccountingUpdate();

    // deauthenticateSessions();
}