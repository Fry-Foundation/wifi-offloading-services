#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../store/config.h"
#include "../store/state.h"
#include "../utils/script_runner.h"
#include "../services/access.h"

#define DEV_PATH "."
#define OPENWRT_PATH "/etc/wayru"
#define SCRIPTS_PATH "/scripts"
#define DATA_PATH "/data"
#define OS_VERSION_FILE "/etc/openwrt_release"
#define PACKAGE_VERSION_FILE "/etc/wayru/VERSION"
#define ID_LENGTH 37

char *initOSVersion(int devEnv)
{
    if (devEnv == 1)
    {
        return strdup("2.0.0");
    }

    FILE *file = fopen(OS_VERSION_FILE, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return NULL;
    }

    char *osVersion = NULL;

    int MAX_LINE_LENGTH = 256;
    char line[MAX_LINE_LENGTH];
    char distrib_id[MAX_LINE_LENGTH];
    char distrib_release[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "DISTRIB_ID", 10) == 0)
        {
            sscanf(line, "DISTRIB_ID='%[^']'", distrib_id);
        }
        else if (strncmp(line, "DISTRIB_RELEASE", 15) == 0)
        {
            sscanf(line, "DISTRIB_RELEASE='%[^']'", distrib_release);
        }
    }

    if (strchr(distrib_release, '\n') != NULL)
    {
        distrib_release[strcspn(distrib_release, "\n")] = 0;
    }

    fclose(file);

    // Allocate memory and copy the version string
    if (*distrib_release != '\0')
    { // Check if distrib_release is not empty
        osVersion = strdup(distrib_release);
        if (osVersion == NULL)
        {
            perror("Memory allocation failed for osVersion");
            fclose(file);
            return NULL;
        }
    }
    else
    {
        fprintf(stderr, "OS version string is empty\n");
    }

    printf("[init] OS version is: %s\n", osVersion);

    return osVersion;
}

char *initServicesVersion(int devEnv)
{
    if (devEnv == 1)
    {
        return strdup("1.0.0");
    }

    FILE *file = fopen(PACKAGE_VERSION_FILE, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return NULL;
    }

    int MAX_LINE_LENGTH = 256;
    char *servicesVersion = NULL;
    char version[MAX_LINE_LENGTH];

    if (fgets(version, MAX_LINE_LENGTH, file) == NULL)
    {
        fprintf(stderr, "Failed to read version\n");
        fclose(file);
        return NULL; // Handle failed read attempt
    }

    if (strchr(version, '\n') != NULL)
    {
        version[strcspn(version, "\n")] = 0;
    }

    fclose(file);

    // Allocate memory for the version string and return
    servicesVersion = strdup(version);
    if (servicesVersion == NULL)
    {
        perror("Memory allocation failed for dynamicVersion");
        return NULL;
    }

    printf("[init] Services version is: %s\n", servicesVersion);

    return servicesVersion;
}

char *initMac(char *scriptsPath)
{
    char scriptFile[256];
    snprintf(scriptFile, sizeof(scriptFile), "%s%s", scriptsPath, "/get-mac.sh");
    char *mac = runScript(scriptFile);
    if (strchr(mac, '\n') != NULL)
    {
        mac[strcspn(mac, "\n")] = 0;
    }

    printf("[init] MAC address is: %s\n", mac);

    return mac;
}

char *initModel(char *scriptsPath)
{
    char scriptFile[256];
    snprintf(scriptFile, sizeof(scriptFile), "%s%s", scriptsPath, "/get-model.sh");
    char *model = runScript(scriptFile);
    if (strchr(model, '\n') != NULL)
    {
        model[strcspn(model, "\n")] = 0;
    }

     printf("[init] Model is: %s\n", model);

    return model;
}

char *initId(char *scriptsPath)
{
    char scriptFile[256];
    snprintf(scriptFile, sizeof(scriptFile), "%s%s", scriptsPath, "/get-uuid.sh");
    char *id = runScript(scriptFile);
    if (strchr(id, '\n') != NULL)
    {
        id[strcspn(id, "\n")] = 0;
    }

    printf("[init] UUID is: %s\n", id);

    return id;
}

void init(int argc, char *argv[])
{
    // Determine if we are running in dev mode
    int devEnv = 0;
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

    // Initialize config
    char *osVersion = initOSVersion(devEnv);
    char *servicesVersion = initServicesVersion(devEnv);
    char *mac = initMac(scriptsPath);
    char *model = initModel(scriptsPath);
    char *id = initId(scriptsPath);

    initConfig(devEnv, basePath, id, mac, model, osVersion, servicesVersion);

    AccessKey *accessKey = initAccessKey();
    initState(0, accessKey);

    // free(basePath);
    // free(id);
    // free(mac);
    // free(model);
    // free(osVersion);
    // free(servicesVersion);
}