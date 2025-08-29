#ifndef MQTT_H
#define MQTT_H

#include "services/callbacks.h"
#include <mosquitto.h>

typedef void (*MessageCallback)(struct mosquitto *mosq, const struct mosquitto_message *message);

// type alias for struct mosquitto
typedef struct mosquitto Mosq;

typedef struct {
    const char *client_id;
    const char *username;
    const char *password;
    const char *broker_url;
    const char *data_path;
    int keepalive;
    int task_interval;
} MqttConfig;

typedef struct {
    Mosq *mosq;
    MqttConfig config;
} MqttClient;

typedef struct MqttTaskContext MqttTaskContext;

Mosq *init_mqtt(const MqttConfig *config);
void refresh_mosquitto_credentials(Mosq *mosq, const char *username);
void publish_mqtt(Mosq *mosq, char *topic, const char *message, int qos);
void subscribe_mqtt(Mosq *mosq, char *topic, int qos, MessageCallback callback);
MqttTaskContext *mqtt_service(Mosq *mosq, const MqttConfig *config);
void cleanup_mqtt(Mosq **mosq);
void clean_mqtt_context(MqttTaskContext *context);

// Function to create MQTT access token refresh callback
AccessTokenCallbacks create_mqtt_token_callbacks(MqttClient *client);

#endif /* MQTT_H */
