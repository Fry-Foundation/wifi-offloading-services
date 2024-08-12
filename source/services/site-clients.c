#include "site-clients.h"
#include "lib/console.h"
#include "services/device-context.h"
#include "services/mqtt.h"
#include "mosquitto.h"
#include <stdio.h>

void connect_callback(struct mosquitto *mosq, const struct mosquitto_message *message) {
    console(CONSOLE_DEBUG, "client connected, payload: %s", (char *)message->payload);
}

void disconnect_callback(struct mosquitto *mosq, const struct mosquitto_message *message) {
    console(CONSOLE_DEBUG, "client disconnected, payload: %s", (char *)message->payload);
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
