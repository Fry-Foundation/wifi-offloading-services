#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <json-c/json.h>
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
#define DEVICE_INFO_FILE "/etc/wayru-os/device.json"

typedef struct
{
    char *name;
    char *brand;
    char *model;
} DeviceInfo;

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

DeviceInfo initDeviceInfo(int dev_env)
{
    DeviceInfo device_info = {0};

    if (dev_env == 1)
    {
        device_info.name = strdup("Genesis");
        device_info.brand = strdup("Wayru");
        device_info.model = strdup("Genesis");
        return device_info;
    }

    FILE *file = fopen(DEVICE_INFO_FILE, "r");
    if (file == NULL)
    {
        perror("[inti] Error opening device info file");
        return device_info;
    }

    // Read the file into a string
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *json_string = malloc(fsize + 1);
    fread(json_string, 1, fsize, file);
    fclose(file);
    json_string[fsize] = 0;

    // Parse the string into a json object
    struct json_object *parsed_json = json_tokener_parse(json_string);
    free(json_string);

    struct json_object *name;
    struct json_object *brand;
    struct json_object *model;

    json_object_object_get_ex(parsed_json, "name", &name);
    json_object_object_get_ex(parsed_json, "brand", &brand);
    json_object_object_get_ex(parsed_json, "model", &model);

    // Copy the values into the device_info struct
    device_info.name = strdup(json_object_get_string(name));
    device_info.brand = strdup(json_object_get_string(brand));
    device_info.model = strdup(json_object_get_string(model));

    // Free the JSON object
    json_object_put(parsed_json);

    printf("[init] Device identifiers are: %s, %s, %s\n", device_info.name, device_info.brand, device_info.model);

    return device_info;
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
            printf("[init] UUID found, took %d attempts.\n", retryCount + 1);
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
    int config_accounting_enabled = -1;
    int config_accounting_interval = -1;
    char config_accounting_api[256] = {'\0'};
    int config_access_task_interval = -1;
    

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

        if (strcmp(argv[i], "--config-accounting-enabled") == 0)
        {
            printf("accounting enable argument: %s\n", argv[i + 1]);
            config_accounting_enabled = atoi(argv[i + 1]);
            continue;
        }

        if (strcmp(argv[i], "--config-accounting-interval") == 0)
        {
            printf("accounting interval argument: %s\n", argv[i + 1]);
            config_accounting_interval = atoi(argv[i + 1]);      
            continue;
        }

        if (strcmp(argv[i], "--config-accounting-api") == 0)
        {
            printf("accounting api argument: %s\n", argv[i + 1]);
            snprintf(config_accounting_api, sizeof(config_accounting_api), "%s", argv[i + 1]);
            continue;
        }

        if (strcmp(argv[i], "--config-access-task-interval") == 0)
        {
            printf("access task interval argument: %s\n", argv[i + 1]);
            config_access_task_interval = atoi(argv[i + 1]);      
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

    if (config_accounting_enabled == -1)
    {
        config_accounting_enabled = atoi(DEFAULT_ACCOUNTING_ENABLED);
    }

    if (config_accounting_interval == -1)
    {
        config_accounting_interval = atoi(DEFAULT_ACCOUNTING_INTERVAL);
    }

    if (config_accounting_api[0] == '\0')
    {
        snprintf(config_accounting_api, sizeof(config_accounting_api), "%s", DEFAULT_ACCOUNTING_API);
    }

    if (config_access_task_interval == -1)
    {
        config_access_task_interval = atoi(DEFAULT_ACCESS_TASK_INTERVAL);
    }

    printf("[init] devEnv: %d\n", devEnv);
    printf("[init] config_enabled: %d\n", config_enabled);
    printf("[init] config_main_api: %s\n", config_main_api);
    printf("[init] config_accounting_enabled: %d\n", config_accounting_enabled);
    printf("[init] config_accounting_interval: %d\n", config_accounting_interval);
    printf("[init] config_accounting_api: %s\n", config_accounting_api);
    printf("[init] config_access_task_interval: %d\n", config_access_task_interval);

    // Set up paths
    char *basePath = (devEnv == 1) ? DEV_PATH : OPENWRT_PATH;
    printf("[init] basePath: %s\n", basePath);

    char scriptsPath[256];
    snprintf(scriptsPath, sizeof(scriptsPath), "%s%s", basePath, "/scripts");

    // Initialize config
    char *osVersion = initOSVersion(devEnv);
    char *servicesVersion = initServicesVersion(devEnv);
    char *mac = initMac(scriptsPath);
    DeviceInfo device_info = initDeviceInfo(devEnv);
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

    initConfig(devEnv, basePath, id, mac, device_info.name, device_info.brand, device_info.model, public_ip, os_name, osVersion, servicesVersion, config_enabled, config_main_api, config_accounting_enabled, config_accounting_interval, config_accounting_api, config_access_task_interval);

    // printf("Valor de config.main_api: %s\n", main_api);

    AccessKey *access_key = init_access_key();
    initState(0, access_key);

    // free(basePath);
    // free(id);
    // free(mac);
    // free(model);
    // free(osVersion);
    // free(servicesVersion);

    free(device_info.name);
    free(device_info.brand);
    free(device_info.model);
}