#include "device_info.h"
#include "config.h"
#include "lib/console.h"
#include "lib/script_runner.h"
#include "services/did-key.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define OS_VERSION_FILE "/etc/openwrt_release"
#define PACKAGE_VERSION_FILE "/etc/wayru-os-services/VERSION"
#define ID_LENGTH 37
#define MAX_RETRIES 50
#define DEVICE_PROFILE_FILE "/etc/wayru-os/device.json"

static Console csl = {
    .topic = "device-info",
    .level = CONSOLE_DEBUG,
};

char *get_os_version() {
    if (config.dev_env) {
        return strdup("23.0.4");
    }

    FILE *file = fopen(OS_VERSION_FILE, "r");
    if (file == NULL) {
        print_error(&csl, "error opening file");
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
            print_error(&csl, "memory allocation failed for os_version");
            fclose(file);
            return NULL;
        }
    } else {
        print_error(&csl, "os_version is empty");
    }

    print_debug(&csl, "os_version is: %s", os_version);

    return os_version;
}

char *get_os_services_version() {
    if (config.dev_env == 1) {
        return strdup("1.0.0");
    }

    FILE *file = fopen(PACKAGE_VERSION_FILE, "r");
    if (file == NULL) {
        print_error(&csl, "error opening services version file");
        return NULL;
    }

    int MAX_LINE_LENGTH = 256;
    char *os_services_version = NULL;
    char version[MAX_LINE_LENGTH];

    if (fgets(version, MAX_LINE_LENGTH, file) == NULL) {
        print_error(&csl, "failed to read services version");
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
        print_error(&csl, "memory allocation failed for services version");
        return NULL;
    }

    print_info(&csl, "services version is: %s", os_services_version);

    return os_services_version;
}

char *get_mac() {
    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s%s", config.scripts_path, "/get-mac.sh");
    char *mac = run_script(script_file);
    if (strchr(mac, '\n') != NULL) {
        mac[strcspn(mac, "\n")] = 0;
    }

    print_debug(&csl, "mac address is: %s", mac);

    return mac;
}

DeviceProfile get_device_profile() {
    DeviceProfile device_profile = {0};

    if (config.dev_env) {
        device_profile.name = strdup("Hemera");
        device_profile.brand = strdup("Wayru");
        device_profile.model = strdup("Genesis");
        return device_profile;
    }

    FILE *file = fopen(DEVICE_PROFILE_FILE, "r");
    if (file == NULL) {
        print_error(&csl, "error opening device info file");
        return device_profile;
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
    device_profile.name = strdup(json_object_get_string(name));
    device_profile.brand = strdup(json_object_get_string(brand));
    device_profile.model = strdup(json_object_get_string(model));

    // Free the JSON object
    json_object_put(parsed_json);

    print_debug(&csl, "device identifiers are: %s, %s, %s", device_profile.name, device_profile.brand,
                device_profile.model);

    return device_profile;
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

            print_debug(&csl, "UUID found; took %d attempts.", retry_count + 1);
            print_debug(&csl, "UUID is: %s", id);

            break; // Exit the loop if a valid UUID is obtained
        }

        print_debug(&csl, "retrying to obtain UUID...");
        sleep(5); // Wait for 5 seconds before retrying
        retry_count++;
    }
    if (retry_count == MAX_RETRIES) {
        print_error(&csl, "unable to obtain UUID after %d attempts. Exiting.", MAX_RETRIES);
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

    print_debug(&csl, "public ip: %s", public_ip);

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

DeviceInfo *init_device_info() {
    DeviceInfo *device_info = malloc(sizeof(DeviceInfo));
    device_info->os_version = get_os_version();
    device_info->os_services_version = get_os_services_version();
    device_info->mac = get_mac();

    DeviceProfile device_profile = get_device_profile();
    device_info->name = device_profile.name;
    device_info->model = device_profile.model;
    device_info->brand = device_profile.brand;

    device_info->device_id = get_id();
    device_info->public_ip = get_public_ip();
    device_info->os_name = get_os_name();
    device_info->did_public_key = get_did_public_key_or_generate_keypair();

    return device_info;
}

void clean_device_info(DeviceInfo *device_info) {
    free(device_info->mac);
    free(device_info->name);
    free(device_info->brand);
    free(device_info->model);
    free(device_info->os_name);
    free(device_info->os_version);
    free(device_info->os_services_version);
    free(device_info->device_id);
    free(device_info->public_ip);
    free(device_info->did_public_key);
}
