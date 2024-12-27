#include "commands.h"
#include "lib/console.h"
#include "services/access_token.h"
#include "services/device_info.h"
#include "services/firmware_upgrade.h"
#include "services/mqtt.h"
#include "services/registration.h"
#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *codename;
    const char *version;
    const char *wayru_device_id;
    AccessToken *access_token;
} FirmwareUpdateCommandContext;

static Console csl = {
    .topic = "commands",
};

static FirmwareUpdateCommandContext firmware_update_command_context = {NULL, NULL, NULL, NULL};

void commands_callback(struct mosquitto *_, const struct mosquitto_message *message) {
    print_debug(&csl, "Received message on commands topic, payload: %s", (char *)message->payload);

    // Parse the JSON payload
    struct json_object *parsed_json;
    struct json_object *command;

    parsed_json = json_tokener_parse((char *)message->payload);
    if (!parsed_json) {
        print_error(&csl, "Failed to parse commands topic payload JSON");
        return;
    }

    if (!json_object_object_get_ex(parsed_json, "command", &command)) {
        print_error(&csl, "Failed to extract command field from commands topic payload JSON");
        json_object_put(parsed_json);
        return;
    }

    const char *command_str = json_object_get_string(command);

    if (strcmp(command_str, "check_firmware_update") == 0) {
        print_info(&csl, "Received firmware update command");
        send_firmware_check_request(firmware_update_command_context.codename, firmware_update_command_context.version,
                                    firmware_update_command_context.wayru_device_id,
                                    firmware_update_command_context.access_token);
    } else {
        print_error(&csl, "Unknown command: %s", command_str);
    }

    // Clean up
    json_object_put(parsed_json);
}

// Subscribe to the commands topic.
// The device will subscribe to "device/<wayru_device_id>/command" to receive commands.
void commands_service(struct mosquitto *mosq,
                      DeviceInfo *device_info,
                      Registration *registration,
                      AccessToken *access_token) {
    // Init firmware update command context
    firmware_update_command_context.codename = device_info->name;
    firmware_update_command_context.version = device_info->os_version;
    firmware_update_command_context.wayru_device_id = registration->wayru_device_id;
    firmware_update_command_context.access_token = access_token;

    // Subscribe to the commands topic
    char commands_topic[256];
    snprintf(commands_topic, sizeof(commands_topic), "device/%s/command", registration->wayru_device_id);
    subscribe_mqtt(mosq, commands_topic, 1, commands_callback);
}
