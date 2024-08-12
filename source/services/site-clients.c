#include "site-clients.h"
#include "lib/console.h"
#include "services/mqtt.h"
#include "mosquitto.h"
#include <stdio.h>

void site_clients_service(struct mosquitto *mosq, char *site) {
    if (site == NULL) {
        console(CONSOLE_DEBUG, "no site to subscribe to");
        return;
    }

    char connect_topic[100];
    snprintf(connect_topic, sizeof(connect_topic), "site/%s/clients/connect", site);
    subscribe_mqtt(mosq, connect_topic, 1);

    char disconnect_topic[100];
    snprintf(disconnect_topic, sizeof(disconnect_topic), "site/%s/clients/disconnect", site);
    subscribe_mqtt(mosq, disconnect_topic, 1);
}
