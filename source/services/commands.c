#include "commands.h"
#include "lib/console.h"
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
} FirmwareUpdateCommandContext;

static FirmwareUpdateCommandContext firmware_update_command_context = {NULL, NULL, NULL};

void commands_callback(struct mosquitto *mosq, const struct mosquitto_message *message) {
    console(CONSOLE_DEBUG, "Received message on commands topic, payload: %s", (char *)message->payload);

    // Parse the JSON payload
    struct json_object *parsed_json;
    struct json_object *command;

    parsed_json = json_tokener_parse((char *)message->payload);
    if (!parsed_json) {
        console(CONSOLE_ERROR, "Failed to parse commands topic payload JSON");
        return;
    }

    if (!json_object_object_get_ex(parsed_json, "command", &command)) {
        console(CONSOLE_ERROR, "Failed to extract command field from commands topic payload JSON");
        json_object_put(parsed_json);
        return;
    }

    const char *command_str = json_object_get_string(command);

    if (strcmp(command_str, "check_firmware_update") == 0) {
        console(CONSOLE_INFO, "Received firmware update command");
        send_firmware_check_request(firmware_update_command_context.codename, firmware_update_command_context.version,
                                    firmware_update_command_context.wayru_device_id);
    } else {
        console(CONSOLE_ERROR, "Unknown command: %s", command_str);
    }

    // Clean up
    json_object_put(parsed_json);
}

// Subscribe to the commands topic.
// The device will subscribe to "device/<wayru_device_id>/command" to receive commands.
void commands_service(struct mosquitto *mosq, DeviceInfo *device_info, Registration *registration) {
    // Init firmware update command context
    firmware_update_command_context.codename = device_info->name;
    firmware_update_command_context.version = device_info->os_version;
    firmware_update_command_context.wayru_device_id = registration->wayru_device_id;

    // Subscribe to the commands topic
    char commands_topic[256];
    snprintf(commands_topic, sizeof(commands_topic), "device/%s/command", registration->wayru_device_id);
    subscribe_mqtt(mosq, commands_topic, 1, commands_callback);
}