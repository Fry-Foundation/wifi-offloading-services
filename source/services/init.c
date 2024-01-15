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
#define MAX_RETRIES 50

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
    int retryCount = 0;

    // Loop indefinitely until a valid UUID is obtained
    while (retryCount < MAX_RETRIES)
    {
        id = run_script(scriptFile);
        // printf("[init] ID: %s\n", id);
        if (id != NULL && strlen(id) > 1 && strncmp(id, "uci", 3) != 0)
        {
            if (strchr(id, '\n') != NULL)
            {
                id[strcspn(id, "\n")] = 0;
            }
            printf("[init] UUID is: %s\n", id);
            break; // Exit the loop if a valid UUID is obtained
        }

        printf("[init] Retrying to obtain UUID...\n");
        sleep(5); // Wait for 5 seconds before retrying
        retryCount++;
    }
    if (retryCount == MAX_RETRIES)
    {
        printf("[init] Error: Unable to obtain UUID after %d attempts. Exiting.\n", MAX_RETRIES);
        exit(1);
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
    int devEnv = 0;
    int config_enabled = -1;
    char config_main_api[256] = {'\0'};
    char config_accounting_api[256] = {'\0'};
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--dev") == 0)
        {
            devEnv = 1;
            continue;
        }

        if (strcmp(argv[i], "--config-enabled") == 0)
        {
            printf("enable argument: %s\n", argv[i + 1]);
            config_enabled = atoi(argv[i + 1]);
            if (config_enabled == 0)
            {
                printf("[init] wayru-os-services is disabled (see \"option enabled\" in config).\n");
                exit(0);
            }

            continue;
        }

        if (strcmp(argv[i], "--config-main-api") == 0)
        {
            printf("main api argument: %s\n", argv[i + 1]);
            snprintf(config_main_api, sizeof(config_main_api), "%s", argv[i + 1]);
            continue;
        }

        if (strcmp(argv[i], "--config-accounting-api") == 0)
        {
            printf("accounting api argument: %s\n", argv[i + 1]);
            snprintf(config_accounting_api, sizeof(config_accounting_api), "%s", argv[i + 1]);
            continue;
        }
    }

    // Set default values for configuration variables
    if (config_enabled == -1)
    {
        config_enabled = atoi(DEFAULT_ENABLED);
    }

    if (config_main_api[0] == '\0')
    {
        snprintf(config_main_api, sizeof(config_main_api), "%s", DEFAULT_MAIN_API);
    }

    if (config_accounting_api[0] == '\0')
    {
        snprintf(config_accounting_api, sizeof(config_accounting_api), "%s", DEFAULT_ACCOUNTING_API);
    }

    printf("[init] devEnv: %d\n", devEnv);
    printf("[init] config_enabled: %d\n", config_enabled);
    printf("[init] config_main_api: %s\n", config_main_api);
    printf("[init] config_accounting_api: %s\n", config_accounting_api);

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
    // int enabled = atoi(DEFAULT_ENABLED);
    // char *main_api = strdup(DEFAULT_MAIN_API);
    // char *accounting_api = strdup(DEFAULT_ACCOUNTING_API);
    //  int enabled = set_default_values();
    //  char *main_api = set_default_values();
    //  char *accounting_api = set_default_values();

    // set_default_values();

    initConfig(devEnv, basePath, id, mac, brand, model, public_ip, os_name, osVersion, servicesVersion, config_enabled, config_main_api, config_accounting_api);

    // printf("Valor de config.main_api: %s\n", main_api);

    AccessKey *accessKey = initAccessKey();
    initState(0, accessKey);

    // free(basePath);
    // free(id);
    // free(mac);
    // free(model);
    // free(osVersion);
    // free(servicesVersion);
}