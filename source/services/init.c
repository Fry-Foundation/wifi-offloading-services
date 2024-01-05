#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../store/config.h"
#include "../store/state.h"
#include "../utils/script_runner.h"
#include "../services/access.h"

#define DEV_PATH "."
#define OPENWRT_PATH "/etc/wayru-os-services"
#define SCRIPTS_PATH "/scripts"
#define DATA_PATH "/data"
#define OS_VERSION_FILE "/etc/openwrt_release"
#define PACKAGE_VERSION_FILE "/etc/wayru-os-services/VERSION"
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
    char *mac = run_script(scriptFile);
    if (strchr(mac, '\n') != NULL)
    {
        mac[strcspn(mac, "\n")] = 0;
    }

    printf("[init] MAC address is: %s\n", mac);

    return mac;
}

char *initBrand(char *scriptsPath)
{
    char scriptFile[256];
    snprintf(scriptFile, sizeof(scriptFile), "%s%s", scriptsPath, "/get-brand.sh");
    char *brand = run_script(scriptFile);
    if (strchr(brand, '\n') != NULL)
    {
        brand[strcspn(brand, "\n")] = 0;
    }

    printf("[init] Brand is: %s\n", brand);

    return brand;
}

char *initModel(char *scriptsPath)
{
    char scriptFile[256];
    snprintf(scriptFile, sizeof(scriptFile), "%s%s", scriptsPath, "/get-model.sh");
    char *model = run_script(scriptFile);
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
    char *id = NULL;

    // Loop indefinitely until a valid UUID is obtained
    while (1)
    {
        id = run_script(scriptFile);
        // if (id != NULL && strlen(id) > 0 && strlen(id) == ID_LENGTH - 1)
        if (id != NULL && strlen(id) > 1)
        {
            if (strchr(id, '\n') != NULL)
            {
                id[strcspn(id, "\n")] = 0;
            }
            printf("[init] UUID is: %s\n", id);
            break; // Exit the loop if a valid UUID is obtained
        }

        printf("[init] Retrying to obtain UUID...\n");
        sleep(1); // Wait for 1 second before retrying
    }

    return id;
}

char *publicIP(char *scriptsPath)
{
    char scriptFile[256];
    snprintf(scriptFile, sizeof(scriptFile), "%s%s", scriptsPath, "/get-public-ip.sh");
    char *public_ip = run_script(scriptFile);
    if (strchr(public_ip, '\n') != NULL)
    {
        public_ip[strcspn(public_ip, "\n")] = 0;
    }

    printf("[init] Public IP: %s\n", public_ip);

    return public_ip;
}

char *initOSName(char *scriptsPath)
{
    char scriptFile[256];
    snprintf(scriptFile, sizeof(scriptFile), "%s%s", scriptsPath, "/get-osname.sh");
    char *os_name = run_script(scriptFile);
    if (strchr(os_name, '\n') != NULL)
    {
        os_name[strcspn(os_name, "\n")] = 0;
    }

    printf("[init] OS name: %s\n", os_name);

    return os_name;
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
    char *brand = initBrand(scriptsPath);
    char *model = initModel(scriptsPath);
    char *public_ip = publicIP(scriptsPath);
    char *os_name = initOSName(scriptsPath);
    char *id = initId(scriptsPath);

    initConfig(devEnv, basePath, id, mac, brand, model, public_ip, os_name, osVersion, servicesVersion);

    AccessKey *accessKey = initAccessKey();
    initState(0, accessKey);

    // free(basePath);
    // free(id);
    // free(mac);
    // free(model);
    // free(osVersion);
    // free(servicesVersion);
}