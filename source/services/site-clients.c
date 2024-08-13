#include "site-clients.h"
#include "lib/console.h"
#include "lib/script_runner.h"
#include "mosquitto.h"
#include "services/config.h"
#include "services/device-context.h"
#include "services/mqtt.h"
#include <stdio.h>
#include <stdlib.h>

// @todo parse mac and session parameters from payload
void connect_callback(struct mosquitto *mosq, const struct mosquitto_message *message) {
    console(CONSOLE_DEBUG, "client connected, payload: %s", (char *)message->payload);

    char script_file[512];
    snprintf(script_file, sizeof(script_file), "%s/%s add %s %s %s %s %s %s %s", config.scripts_path,
             "nds-preemptive-list.lua", "00:11:22:33:44:55", "3600", "100", "100", "100", "100", "custom_data");
    char *output = run_script(script_file);
    free(output);
}

// @todo parse mac from payload
void disconnect_callback(struct mosquitto *mosq, const struct mosquitto_message *message) {
    console(CONSOLE_DEBUG, "client disconnected, payload: %s", (char *)message->payload);

    char script_file[512];
    snprintf(script_file, sizeof(script_file), "%s/%s remove %s", config.scripts_path, "nds-preemptive-list.lua",
             "00:11:22:33:44:55");
}

// @todo configure site mac
void site_clients_service(struct mosquitto *mosq, Site *site) {
    if (site == NULL || site->id == NULL) {
        console(CONSOLE_DEBUG, "no site to subscribe to");
        return;
    }

    char connect_topic[100];
    snprintf(connect_topic, sizeof(connect_topic), "site/%s/clients/connect", site->id);
    subscribe_mqtt(mosq, connect_topic, 1, connect_callback);

    char disconnect_topic[100];
    snprintf(disconnect_topic, sizeof(disconnect_topic), "site/%s/clients/disconnect", site->id);
    subscribe_mqtt(mosq, disconnect_topic, 1, disconnect_callback);
}
