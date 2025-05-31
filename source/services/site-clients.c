#include "site-clients.h"
#include "lib/console.h"
#include "lib/script_runner.h"
#include "services/config/config.h"
#include "services/device-context.h"
#include "services/mqtt/mqtt.h"
#include "services/nds.h"
#include <fcntl.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define SESSION_TIMEOUT "60"
#define UPLOAD_RATE "0"
#define DOWNLOAD_RATE "0"
#define UPLOAD_QUOTA "0"
#define DOWNLOAD_QUOTA "0"
#define CUSTOM "custom_placeholder"

static Console csl = {
    .topic = "site-clients",
};

void handle_connect(const char *mac) {
    // Build the command
    char command[512];
    snprintf(command, sizeof(command), "%s/%s add %s %s %s %s %s %s %s", config.scripts_path,
             "nds-set-preemptive-list.lua", mac, SESSION_TIMEOUT, UPLOAD_RATE, DOWNLOAD_RATE, UPLOAD_QUOTA,
             DOWNLOAD_QUOTA, CUSTOM);

    print_debug(&csl, "Command: %s", command);

    // Run the script
    char *output = run_script(command);
    print_debug(&csl, "Script output: %s", output);

    // Clean up
    free(output);
}

void handle_disconnect(const char *mac) {
    // Build the command
    char command[512];
    snprintf(command, sizeof(command), "%s/%s remove %s", config.scripts_path, "nds-set-preemptive-list.lua", mac);

    // Run the script
    char *output = run_script(command);
    print_debug(&csl, "Script output: %s", output);

    // Clean up
    free(output);
}

void site_clients_callback(Mosq *_, const struct mosquitto_message *message) {
    print_debug(&csl, "Received message on site clients topic, payload: %s", (char *)message->payload);

    json_object *events_array = json_tokener_parse((char *)message->payload);
    if (events_array == NULL) {
        print_error(&csl, "Failed to parse site clients topic payload JSON");
        return;
    }

    if (json_object_get_type(events_array) != json_type_array) {
        print_error(&csl, "Expected JSON array in site clients topic payload");
        json_object_put(events_array);
        return;
    }

    int events_count = json_object_array_length(events_array);
    for (int i = 0; i < events_count; i++) {
        // Get each event string from the array
        json_object *event_json = json_object_array_get_idx(events_array, i);
        if (!event_json) {
            print_warn(&csl, "Could not get event JSON object from array");
            continue;
        }

        const char *event_str = json_object_get_string(event_json);
        if (!event_str) {
            print_warn(&csl, "Could not get event string from JSON object");
            continue;
        }

        // Parse the event string
        // The event string is formatted as: "method=client_deauth, clientmac=..., ..."
        // For this process we only care about the method and the mac
        // Possible methods are:
        //   client_auth, client_deauth, idle_deauth, timeout_deauth,
        //   downquota_deauth, upquota_deauth, ndsctl_auth, ndsctl_deauth, shutdown_deauth
        //
        // See nds-binauth.sh in the openwrt/ scripts folder for more details

        // Get the method
        const char *method_key = "method=";
        const char *method_ptr = strstr(event_str, method_key);
        if (!method_ptr) {
            print_warn(&csl, "Could not find method in event string");
            continue;
        }
        method_ptr += strlen(method_key);
        char method[64] = {0};
        int j = 0;
        while (method_ptr[j] != '\0' && method_ptr[j] != ',' && j < (int)(sizeof(method) - 1)) {
            method[j] = method_ptr[j];
            j++;
        }
        method[j] = '\0';

        // Get the mac
        const char *mac_key = "clientmac=";
        const char *mac_ptr = strstr(event_str, mac_key);
        if (!mac_ptr) {
            print_warn(&csl, "Could not find mac in event string");
            continue;
        }
        mac_ptr += strlen(mac_key);
        char mac[18] = {0};
        j = 0;
        while (mac_ptr[j] != '\0' && mac_ptr[j] != ',' && j < (int)(sizeof(mac) - 1)) {
            mac[j] = mac_ptr[j];
            j++;
        }
        mac[j] = '\0';

        // Call the appropriate handler based on the method
        if (strcmp(method, "client_auth") == 0 || strcmp(method, "ndsctl_auth") == 0) {
            handle_connect(mac);
        } else {
            handle_disconnect(mac);
        }
    }
}

void configure_site_mac(char *mac) {
    // Build the command
    char command[512];
    snprintf(command, sizeof(command), "%s/%s %s", config.scripts_path, "network-set-mac.lua", mac);

    // Run the script
    char *output = run_script(command);
    print_debug(&csl, "Script output: %s", output);

    // Clean up
    free(output);
}

void init_site_clients(Mosq *mosq, Site *site, NdsClient *nds_client) {
    if (site == NULL || site->id == NULL || site->mac == NULL) {
        print_info(&csl, "no site to subscribe to or incomplete details");
        return;
    }

    if (config.dev_env) {
        return;
    }

    if (nds_client->opennds_installed == false) {
        print_warn(&csl, "OpenNDS is not installed, skipping site clients service");
        return;
    }

    configure_site_mac(site->mac);

    char site_clients_topic[256];
    snprintf(site_clients_topic, sizeof(site_clients_topic), "site/%s/clients", site->id);
    subscribe_mqtt(mosq, site_clients_topic, 1, site_clients_callback);
}
