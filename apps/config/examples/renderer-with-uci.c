#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include "uci/uci.h"

int set_uci_option(struct uci_context *ctx, const char *package, const char *section, 
                   const char *option, const char *value) {
    struct uci_ptr ptr;
    char path[256];
    
    // Build the UCI path string
    snprintf(path, sizeof(path), "%s.%s.%s=%s", package, section, option, value);
    
    // Parse the path
    if (uci_lookup_ptr(ctx, &ptr, path, true) != UCI_OK) {
        fprintf(stderr, "Error: Failed to parse UCI path: %s\n", path);
        return -1;
    }
    
    // Set the value
    if (uci_set(ctx, &ptr) != UCI_OK) {
        fprintf(stderr, "Error: Failed to set UCI option: %s\n", path);
        return -1;
    }
    
    return 0;
}

int apply_wireless_interfaces(struct uci_context *ctx, json_object *config_obj) {
    json_object *wireless_obj = NULL;
    if (!json_object_object_get_ex(config_obj, "wireless", &wireless_obj)) {
        fprintf(stderr, "No wireless configuration found\n");
        return -1;
    }

    json_object *interfaces_array = NULL;
    if (!json_object_object_get_ex(wireless_obj, "interfaces", &interfaces_array)) {
        fprintf(stderr, "No wireless interfaces found\n");
        return -1;
    }

    int interfaces_count = json_object_array_length(interfaces_array);
    
    for (int i = 0; i < interfaces_count; i++) {
        json_object *interface_obj = json_object_array_get_idx(interfaces_array, i);
        if (!interface_obj) continue;

        // Extract interface properties
        json_object *name_obj, *device_obj, *network_obj, *mode_obj, *ssid_obj, 
                   *encryption_obj, *disabled_obj, *key_obj, *isolate_obj;

        const char *name = NULL, *device = NULL, *network = NULL, *mode = NULL,
                   *ssid = NULL, *encryption = NULL, *key = NULL;
        int disabled = 0, isolate = 0;

        if (json_object_object_get_ex(interface_obj, "name", &name_obj)) {
            name = json_object_get_string(name_obj);
        }
        if (json_object_object_get_ex(interface_obj, "device", &device_obj)) {
            device = json_object_get_string(device_obj);
        }
        if (json_object_object_get_ex(interface_obj, "network", &network_obj)) {
            network = json_object_get_string(network_obj);
        }
        if (json_object_object_get_ex(interface_obj, "mode", &mode_obj)) {
            mode = json_object_get_string(mode_obj);
        }
        if (json_object_object_get_ex(interface_obj, "ssid", &ssid_obj)) {
            ssid = json_object_get_string(ssid_obj);
        }
        if (json_object_object_get_ex(interface_obj, "encryption", &encryption_obj)) {
            encryption = json_object_get_string(encryption_obj);
        }
        if (json_object_object_get_ex(interface_obj, "disabled", &disabled_obj)) {
            disabled = json_object_get_boolean(disabled_obj);
        }
        if (json_object_object_get_ex(interface_obj, "key", &key_obj)) {
            key = json_object_get_string(key_obj);
        }
        if (json_object_object_get_ex(interface_obj, "isolate", &isolate_obj)) {
            isolate = json_object_get_boolean(isolate_obj);
        }

        // Apply UCI settings
        if (name) {
            printf("Configuring wireless interface: %s\n", name);
            
            if (device && set_uci_option(ctx, "wireless", name, "device", device) != 0) {
                return -1;
            }
            if (network && set_uci_option(ctx, "wireless", name, "network", network) != 0) {
                return -1;
            }
            if (mode && set_uci_option(ctx, "wireless", name, "mode", mode) != 0) {
                return -1;
            }
            if (ssid && set_uci_option(ctx, "wireless", name, "ssid", ssid) != 0) {
                return -1;
            }
            if (encryption && set_uci_option(ctx, "wireless", name, "encryption", encryption) != 0) {
                return -1;
            }
            
            // Handle boolean values
            const char *disabled_str = disabled ? "1" : "0";
            if (set_uci_option(ctx, "wireless", name, "disabled", disabled_str) != 0) {
                return -1;
            }
            
            if (key && set_uci_option(ctx, "wireless", name, "key", key) != 0) {
                return -1;
            }
            
            const char *isolate_str = isolate ? "1" : "0";
            if (set_uci_option(ctx, "wireless", name, "isolate", isolate_str) != 0) {
                return -1;
            }
            
            printf("Successfully configured interface: %s\n", name);
        }
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    const char *config_file = "config.example.json";
    struct uci_context *ctx = NULL;
    int ret = 0;
    
    if (argc > 1) {
        config_file = argv[1];
    }

    // Initialize UCI context
    ctx = uci_alloc_context();
    if (!ctx) {
        fprintf(stderr, "Error: Failed to allocate UCI context\n");
        return 1;
    }

    // Read JSON file
    FILE *fp = fopen(config_file, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open config file '%s'\n", config_file);
        ret = 1;
        goto cleanup;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Read file content
    char *json_string = malloc(file_size + 1);
    if (!json_string) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(fp);
        ret = 1;
        goto cleanup;
    }

    size_t read_size = fread(json_string, 1, file_size, fp);
    json_string[read_size] = '\0';
    fclose(fp);

    // Parse JSON
    json_object *root_obj = json_tokener_parse(json_string);
    free(json_string);

    if (!root_obj) {
        fprintf(stderr, "Error: Failed to parse JSON\n");
        ret = 1;
        goto cleanup;
    }

    // Get config object
    json_object *config_obj = NULL;
    if (!json_object_object_get_ex(root_obj, "config", &config_obj)) {
        fprintf(stderr, "Error: No 'config' object found in JSON\n");
        json_object_put(root_obj);
        ret = 1;
        goto cleanup;
    }

    // Apply wireless interfaces configuration
    if (apply_wireless_interfaces(ctx, config_obj) != 0) {
        fprintf(stderr, "Error: Failed to apply wireless configuration\n");
        json_object_put(root_obj);
        ret = 1;
        goto cleanup;
    }

    // Load wireless package for commit
    struct uci_package *pkg = NULL;
    if (uci_load(ctx, "wireless", &pkg) != UCI_OK) {
        fprintf(stderr, "Error: Failed to load wireless package\n");
        json_object_put(root_obj);
        ret = 1;
        goto cleanup;
    }

    // Save changes
    if (uci_save(ctx, pkg) != UCI_OK) {
        fprintf(stderr, "Error: Failed to save wireless configuration\n");
        json_object_put(root_obj);
        ret = 1;
        goto cleanup;
    }

    // Commit changes
    if (uci_commit(ctx, &pkg, false) != UCI_OK) {
        fprintf(stderr, "Error: Failed to commit wireless configuration\n");
        json_object_put(root_obj);
        ret = 1;
        goto cleanup;
    }

    printf("Wireless configuration applied and committed successfully\n");

    // Cleanup JSON
    json_object_put(root_obj);

cleanup:
    // Free UCI context
    if (ctx) {
        uci_free_context(ctx);
    }
    
    return ret;
}