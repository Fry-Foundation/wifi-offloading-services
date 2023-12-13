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