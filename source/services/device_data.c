#include "device_data.h"
#include "config.h"
#include "lib/console.h"
#include "lib/script_runner.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define OS_VERSION_FILE "/etc/openwrt_release"
#define PACKAGE_VERSION_FILE "/etc/wayru-os-services/VERSION"
#define ID_LENGTH 37
#define MAX_RETRIES 50
#define DEVICE_INFO_FILE "/etc/wayru-os/device.json"

DeviceData device_data = {0};

char *get_os_version() {
    if (config.dev_env) {
        return strdup("2.0.0");
    }

    FILE *file = fopen(OS_VERSION_FILE, "r");
    if (file == NULL) {
        console(CONSOLE_ERROR, "error opening OS version file");
        perror("error opening file");
        return NULL;
    }

    char *os_version = NULL;

    int MAX_LINE_LENGTH = 256;
    char line[MAX_LINE_LENGTH];
    char distrib_id[MAX_LINE_LENGTH];
    char distrib_release[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "DISTRIB_ID", 10) == 0) {
            sscanf(line, "DISTRIB_ID='%[^']'", distrib_id);
        } else if (strncmp(line, "DISTRIB_RELEASE", 15) == 0) {
            sscanf(line, "DISTRIB_RELEASE='%[^']'", distrib_release);
        }
    }

    if (strchr(distrib_release, '\n') != NULL) {
        distrib_release[strcspn(distrib_release, "\n")] = 0;
    }

    fclose(file);

    // Allocate memory and copy the version string
    if (*distrib_release != '\0') { // Check if distrib_release is not empty
        os_version = strdup(distrib_release);
        if (os_version == NULL) {
            console(CONSOLE_ERROR, "failed to allocate memory for OS version");
            perror("memory allocation failed for osVersion");
            fclose(file);
            return NULL;
        }
    } else {
        console(CONSOLE_ERROR, "OS version is empty");
    }

    console(CONSOLE_DEBUG, "OS version is: %s", os_version);

    return os_version;
}

char *get_os_services_version() {
    if (config.dev_env == 1) {
        return strdup("1.0.0");
    }

    FILE *file = fopen(PACKAGE_VERSION_FILE, "r");
    if (file == NULL) {
        console(CONSOLE_ERROR, "error opening services version file");
        perror("error opening file");
        return NULL;
    }

    int MAX_LINE_LENGTH = 256;
    char *os_services_version = NULL;
    char version[MAX_LINE_LENGTH];

    if (fgets(version, MAX_LINE_LENGTH, file) == NULL) {
        console(CONSOLE_ERROR, "failed to read services version");
        fclose(file);
        return NULL; // Handle failed read attempt
    }

    if (strchr(version, '\n') != NULL) {
        version[strcspn(version, "\n")] = 0;
    }

    fclose(file);

    // Allocate memory for the version string and return
    os_services_version = strdup(version);
    if (os_services_version == NULL) {
        console(CONSOLE_ERROR, "memory allocation failed for services version");
        perror("memory allocation failed for dynamicVersion");
        return NULL;
    }

    console(CONSOLE_ERROR, "services version is: %s", os_services_version);

    return os_services_version;
}

char *get_mac() {
    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s%s", config.scripts_path, "/get-mac.sh");
    char *mac = run_script(script_file);
    if (strchr(mac, '\n') != NULL) {
        mac[strcspn(mac, "\n")] = 0;
    }

    console(CONSOLE_DEBUG, "mac address is: %s", mac);

    return mac;
}

DeviceInfo get_device_info() {
    DeviceInfo device_info = {0};

    if (config.dev_env) {
        device_info.name = strdup("Genesis");
        device_info.brand = strdup("Wayru");
        device_info.model = strdup("Genesis");
        return device_info;
    }

    FILE *file = fopen(DEVICE_INFO_FILE, "r");
    if (file == NULL) {
        console(CONSOLE_ERROR, "error opening device info file");
        perror("error opening device info file");
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

    console(CONSOLE_DEBUG, "device identifiers are: %s, %s, %s", device_info.name, device_info.brand,
            device_info.model);

    return device_info;
}

char *get_id() {
    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s%s", config.scripts_path, "/get-uuid.sh");
    char *id = NULL;
    int retry_count = 0;

    // Loop indefinitely until a valid UUID is obtained
    while (retry_count < MAX_RETRIES) {
        id = run_script(script_file);
        if (id != NULL && strlen(id) > 1 && strncmp(id, "uci", 3) != 0) {
            if (strchr(id, '\n') != NULL) {
                id[strcspn(id, "\n")] = 0;
            }

            console(CONSOLE_DEBUG, "UUID found, took %d attempts.", retry_count + 1);
            console(CONSOLE_DEBUG, "UUID is: %s", id);

            break; // Exit the loop if a valid UUID is obtained
        }

        console(CONSOLE_DEBUG, "retrying to obtain UUID...");
        sleep(5); // Wait for 5 seconds before retrying
        retry_count++;
    }
    if (retry_count == MAX_RETRIES) {
        console(CONSOLE_ERROR, "unable to obtain UUID after %d attempts. Exiting.", MAX_RETRIES);
        exit(1);
    }

    return id;
}

char *get_public_ip() {
    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s%s", config.scripts_path, "/get-public-ip.sh");
    char *public_ip = run_script(script_file);
    if (strchr(public_ip, '\n') != NULL) {
        public_ip[strcspn(public_ip, "\n")] = 0;
    }

    console(CONSOLE_DEBUG, "public ip: %s", public_ip);

    return public_ip;
}

char *get_os_name() {
    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s%s", config.scripts_path, "/get-osname.sh");
    char *os_name = run_script(script_file);
    if (strchr(os_name, '\n') != NULL) {
        os_name[strcspn(os_name, "\n")] = 0;
    }

    return os_name;
}

void init_device_data() {
    device_data.os_version = get_os_version();
    device_data.os_services_version = get_os_services_version();
    device_data.mac = get_mac();

    DeviceInfo device_info = get_device_info();
    device_data.name = device_info.name;
    device_data.model = device_info.model;
    device_data.brand = device_info.brand;

    device_data.device_id = get_id();
    device_data.public_ip = get_public_ip();
    device_data.os_name = get_os_name();
}

void clean_device_data() {
    free(device_data.mac);
    free(device_data.name);
    free(device_data.brand);
    free(device_data.model);
    free(device_data.os_name);
    free(device_data.os_version);
    free(device_data.os_services_version);
    free(device_data.device_id);
    free(device_data.public_ip);
}
