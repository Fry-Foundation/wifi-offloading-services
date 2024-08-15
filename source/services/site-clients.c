#include "site-clients.h"
#include "lib/console.h"
#include "lib/script_runner.h"
#include "mosquitto.h"
#include "services/config.h"
#include "services/device-context.h"
#include "services/mqtt.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>

void connect_callback(struct mosquitto *mosq, const struct mosquitto_message *message) {
    console(CONSOLE_DEBUG, "client connected, payload: %s", (char *)message->payload);

    // Parse the JSON payload
    struct json_object *parsed_json;
    struct json_object *mac;
    struct json_object *sessiontimeout;
    struct json_object *uploadrate;
    struct json_object *downloadrate;
    struct json_object *uploadquota;
    struct json_object *downloadquota;
    struct json_object *custom;

    parsed_json = json_tokener_parse((char *)message->payload);
    if (!parsed_json) {
        console(CONSOLE_ERROR, "Failed to parse JSON");
        return;
    }

    // Extract values from the JSON object
    json_object_object_get_ex(parsed_json, "mac", &mac);
    json_object_object_get_ex(parsed_json, "sessiontimeout", &sessiontimeout);
    json_object_object_get_ex(parsed_json, "uploadrate", &uploadrate);
    json_object_object_get_ex(parsed_json, "downloadrate", &downloadrate);
    json_object_object_get_ex(parsed_json, "uploadquota", &uploadquota);
    json_object_object_get_ex(parsed_json, "downloadquota", &downloadquota);
    json_object_object_get_ex(parsed_json, "custom", &custom);

    // Build the command
    char script_file_and_command[512];
    snprintf(script_file_and_command, sizeof(script_file_and_command), "%s/%s add %s %s %s %s %s %s %s",
             config.scripts_path, "nds-preemptive-list.lua", json_object_get_string(mac),
             json_object_get_string(sessiontimeout), json_object_get_string(uploadrate),
             json_object_get_string(downloadrate), json_object_get_string(uploadquota),
             json_object_get_string(downloadquota), json_object_get_string(custom));

    // Run the script
    char *output = run_script(script_file_and_command);
    console(CONSOLE_DEBUG, "Script output: %s", output);

    // Clean up
    free(output);
    json_object_put(parsed_json);
}

void disconnect_callback(struct mosquitto *mosq, const struct mosquitto_message *message) {
    console(CONSOLE_DEBUG, "client disconnected, payload: %s", (char *)message->payload);

    // Parse the JSON payload
    struct json_object *parsed_json;
    struct json_object *mac;

    parsed_json = json_tokener_parse((char *)message->payload);
    if (!parsed_json) {
        console(CONSOLE_ERROR, "Failed to parse JSON");
        return;
    }

    // Extract values from the JSON object
    json_object_object_get_ex(parsed_json, "mac", &mac);

    // Build the command
    char script_file_and_command[512];
    snprintf(script_file_and_command, sizeof(script_file_and_command), "%s/%s remove %s", config.scripts_path,
             "nds-preemptive-list.lua", json_object_get_string(mac));

    // Run the script
    char *output = run_script(script_file_and_command);
    console(CONSOLE_DEBUG, "Script output: %s", output);

    // Clean up
    free(output);
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

// @todo configure site mac
void site_clients_service(struct mosquitto *mosq, Site *site) {
    if (site == NULL || site->id == NULL || site->mac == NULL) {
        console(CONSOLE_INFO, "no site to subscribe to or incomplete details");
        return;
    }

    configure_site_mac(site->mac);

    char connect_topic[100];
    snprintf(connect_topic, sizeof(connect_topic), "site/%s/clients/connect", site->id);
    subscribe_mqtt(mosq, connect_topic, 1, connect_callback);

    char disconnect_topic[100];
    snprintf(disconnect_topic, sizeof(disconnect_topic), "site/%s/clients/disconnect", site->id);
    subscribe_mqtt(mosq, disconnect_topic, 1, disconnect_callback);
}
