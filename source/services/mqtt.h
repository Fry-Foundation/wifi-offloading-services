#ifndef MQTT_H
#define MQTT_H

#include <mosquitto.h>
#include "services/access_token.h"

struct mosquitto *init_mqtt(AccessToken *access_token);
void publish_mqtt(struct mosquitto *mosq, char *topic, const char *message);
void subscribe_mqtt(struct mosquitto *mosq, char *topic, int qos);
void clean_up_mosquitto(struct mosquitto **mosq);

#endif /* MQTT_H */
