#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../lib/base64.h"
#include "../store/config.h"
#include "../store/state.h"
#include "../utils/script_runner.h"

#define DEV_PATH "."
#define OPENWRT_PATH "/etc/wayru"
#define SCRIPTS_PATH "/scripts"
#define DATA_PATH "/data"
#define OS_VERSION_FILE "/etc/openwrt_release"
#define PACKAGE_VERSION_FILE "/etc/wayru/VERSION"

char *initOSVersion(int devEnv)
{
    if (devEnv == 1)
    {
        return strdup("1.0.0");
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

    printf("Services Version is: %s\n", version);

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

    printf("Dynamic version is: %s\n", servicesVersion);

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

    return strdup("00:00:00:00:00:00");
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

    return model;
}

char *initId(char *mac, char *model)
{
    // Calculate the length of the combined string
    int combinedLength = strlen(mac) + strlen(model) + 2; // +1 for hyphen, +1 for null terminator
    char *combined = (char *)malloc(combinedLength);
    if (combined == NULL)
    {
        perror("Failed to allocate memory for combined string");
        return NULL;
    }

    // Concatenate MAC and model with a hyphen
    snprintf(combined, combinedLength, "%s-%s", mac, model);

    // Calculate the length of the encoded string
    int encodedLength = Base64encode_len(combinedLength);

    // Allocate memory for the encoded string
    char *encoded = (char *)malloc(encodedLength);
    if (encoded == NULL)
    {
        perror("Failed to allocate memory for encoded data");
        free(combined);
        return NULL;
    }

    // Encode the combined string
    Base64encode(encoded, combined, combinedLength - 1); // -1 to exclude null terminator

    free(combined);
    return encoded;
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
    char *id = initId(mac, model);

    initConfig(devEnv, basePath, id, mac, model, osVersion, servicesVersion);
    initState(0);
    printf("[init] here\n");

    // free(basePath);
    // free(id);
    // free(mac);
    // free(model);
    // free(osVersion);
    // free(servicesVersion);
}