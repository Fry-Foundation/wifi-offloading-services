#ifndef MQTT_H
#define MQTT_H

#include "services/access_token.h"
#include "services/registration.h"
#include <mosquitto.h>

typedef void (*MessageCallback)(struct mosquitto *mosq, const struct mosquitto_message *message);

// type alias for struct mosquitto
typedef struct mosquitto Mosq;

Mosq *init_mqtt(Registration *registration, AccessToken *access_token);
void refresh_mosquitto_access_token(Mosq *mosq, AccessToken *access_token);
void publish_mqtt(Mosq *mosq, char *topic, const char *message, int qos);
void subscribe_mqtt(Mosq *mosq, char *topic, int qos, MessageCallback callback);
void mqtt_service(Scheduler *sch, Mosq *mosq, Registration *registration, AccessToken *access_token);
void cleanup_mqtt(Mosq **mosq);

#endif /* MQTT_H */
