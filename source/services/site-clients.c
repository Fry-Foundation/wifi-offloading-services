#include "site-clients.h"
#include "lib/console.h"
#include "lib/script_runner.h"
#include "mosquitto.h"
#include "services/config.h"
#include "services/device-context.h"
#include "services/mqtt.h"
#include <json-c/json.h>
#include <json-c/json_types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void handle_connect(struct json_object *parsed_json) {
    struct json_object *mac;
    struct json_object *sessiontimeout;
    struct json_object *uploadrate;
    struct json_object *downloadrate;
    struct json_object *uploadquota;
    struct json_object *downloadquota;
    struct json_object *custom;

    // Extract the expected values from the JSON object
    json_object_object_get_ex(parsed_json, "mac", &mac);
    json_object_object_get_ex(parsed_json, "sessiontimeout", &sessiontimeout);
    json_object_object_get_ex(parsed_json, "uploadrate", &uploadrate);
    json_object_object_get_ex(parsed_json, "downloadrate", &downloadrate);
    json_object_object_get_ex(parsed_json, "uploadquota", &uploadquota);
    json_object_object_get_ex(parsed_json, "downloadquota", &downloadquota);
    json_object_object_get_ex(parsed_json, "custom", &custom);

    // Build the command
    char command[512];
    snprintf(command, sizeof(command), "%s/%s add %s %s %s %s %s %s %s", config.scripts_path, "nds-preemptive-list.lua",
             json_object_get_string(mac), json_object_get_string(sessiontimeout), json_object_get_string(uploadrate),
             json_object_get_string(downloadrate), json_object_get_string(uploadquota),
             json_object_get_string(downloadquota), json_object_get_string(custom));

    // Run the script
    char *output = run_script(command);
    console(CONSOLE_DEBUG, "Script output: %s", output);

    // Clean up
    free(output);
}

void handle_disconnect(struct json_object *parsed_json) {
    struct json_object *mac;

    // Extract values from the JSON object
    json_object_object_get_ex(parsed_json, "mac", &mac);

    // Build the command
    char command[512];
    snprintf(command, sizeof(command), "%s/%s remove %s", config.scripts_path, "nds-preemptive-list.lua",
             json_object_get_string(mac));

    // Run the script
    char *output = run_script(command);
    console(CONSOLE_DEBUG, "Script output: %s", output);

    // Clean up
    free(output);
}

void site_clients_callback(struct mosquitto *mosq, const struct mosquitto_message *message) {
    console(CONSOLE_DEBUG, "Received message on site clients topic, payload: %s", (char *)message->payload);

    // Parse the JSON payload
    struct json_object *parsed_json;
    struct json_object *type;

    parsed_json = json_tokener_parse((char *)message->payload);
    if (!parsed_json) {
        console(CONSOLE_ERROR, "Failed to parse clients topic payload JSON");
        return;
    }

    if (!json_object_object_get_ex(parsed_json, "type", &type)) {
        console(CONSOLE_ERROR, "Failed to extract type field from clients topic payload JSON");
        json_object_put(parsed_json);
        return;
    }

    const char *type_str = json_object_get_string(type);

    // Handle the message based on the type
    if (strcmp(type_str, "connect") == 0) {
        handle_connect(parsed_json);
    } else if (strcmp(type_str, "disconnect") == 0) {
        handle_disconnect(parsed_json);
    } else {
        console(CONSOLE_ERROR, "Unknown clients topic type: %s", type_str);
    }

    // Clean up
    json_object_put(parsed_json);
}

void configure_site_mac(char *mac) {
    // Build the command
    char command[512];
    snprintf(command, sizeof(command), "%s/%s %s", config.scripts_path, "network-mac.lua", mac);

    // Run the script
    char *output = run_script(command);
    console(CONSOLE_DEBUG, "Script output: %s", output);

    // Clean up
    free(output);
}

void site_clients_service(struct mosquitto *mosq, Site *site) {
    if (site == NULL || site->id == NULL || site->mac == NULL) {
        console(CONSOLE_INFO, "no site to subscribe to or incomplete details");
        return;
    }

    configure_site_mac(site->mac);

    char site_clients_topic[256];
    snprintf(site_clients_topic, sizeof(site_clients_topic), "site/%s/clients", site->id);
    subscribe_mqtt(mosq, site_clients_topic, 1, site_clients_callback);
}
