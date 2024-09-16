#include "mqtt.h"
#include "services/access_token.h"
#include "services/env.h"
#include "services/registration.h"
#include <lib/console.h>
#include <mosquitto.h>
#include <services/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CA_FILE_NAME "ca.crt"
#define KEY_FILE_NAME "device.key"
#define CSR_FILE_NAME "device.csr"
#define CERT_FILE_NAME "device.crt"
#define MAX_TOPIC_CALLBACKS 10

typedef struct {
    char *topic;
    MessageCallback callback;
} TopicCallback;

static TopicCallback topic_callbacks[MAX_TOPIC_CALLBACKS];
static int topic_callbacks_count = 0;

void on_connect(struct mosquitto *mosq, void *obj, int reason_code) {
    console(CONSOLE_DEBUG, "MQTT client on_connect callback, reason_code: %d", reason_code);

    if (reason_code) {
        console(CONSOLE_ERROR, "Error: Unable to connect to the broker. %s", mosquitto_connack_string(reason_code));
        // exit (1);
    } else {
        console(CONSOLE_INFO, "Connected to the broker.");
    }
}

void on_disconnect(struct mosquitto *mosq, void *obj, int reason_code) {
    console(CONSOLE_INFO, "Disconnected from the broker. Reason code: %d", reason_code);
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    for (int i = 0; i < topic_callbacks_count; i++) {
        if (strcmp(topic_callbacks[i].topic, msg->topic) == 0) {
            topic_callbacks[i].callback(mosq, msg);
        }
    }
}

void on_publish(struct mosquitto *mosq, void *obj, int mid) {
    console(CONSOLE_INFO, "Message has been published, mid %d", mid);
}

void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos, const int *granted_qos) {
    console(CONSOLE_INFO, "Subscribed to a topic, mid %d", mid);
}

void subscribe_mqtt(struct mosquitto *mosq, char *topic, int qos, MessageCallback callback) {
    if (topic_callbacks_count >= MAX_TOPIC_CALLBACKS) {
        console(CONSOLE_ERROR, "Error: Maximum number of topic callbacks reached.");
        return;
    }

    int rc = mosquitto_subscribe(mosq, NULL, topic, qos);
    if (rc != MOSQ_ERR_SUCCESS) {
        console(CONSOLE_ERROR, "Error: Unable to subscribe to the topic. %s", mosquitto_strerror(rc));
    } else {
        console(CONSOLE_INFO, "Subscribed to the topic %s successfully.", topic);
        topic_callbacks[topic_callbacks_count].topic = strdup(topic);
        topic_callbacks[topic_callbacks_count].callback = callback;
        topic_callbacks_count++;
    }
}

void publish_mqtt(struct mosquitto *mosq, char *topic, const char *message, int qos) {
    int rc = mosquitto_publish(mosq, NULL, topic, strlen(message), message, qos, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        console(CONSOLE_ERROR, "Error: Unable to publish message. %s\n", mosquitto_strerror(rc));
    }
}

struct mosquitto *init_mosquitto(Registration *registration, AccessToken *access_token) {
    // Load user and password from env file
    // If present, these override the access token
    char env_file[256];
    snprintf(env_file, sizeof(env_file), "%s%s", config.data_path, "/.env");
    load_env(env_file);
    const char *env_mqtt_user = env("MQTT_USER");
    const char *env_mqtt_password = env("MQTT_PASS");
    const char *mqtt_user = (env_mqtt_user && strlen(env_mqtt_user) > 0) ? env_mqtt_user : access_token->token;
    const char *mqtt_password = (env_mqtt_password && strlen(env_mqtt_password) > 0) ? env_mqtt_password : "any";
    console(CONSOLE_DEBUG, "user is %s", mqtt_user);
    console(CONSOLE_DEBUG, "password is %s", mqtt_password);

    struct mosquitto *mosq;
    int rc;
    int pw_set;
    int tls_set;
    int tls_opts_set;
    // Initialize the Mosquitto library
    mosquitto_lib_init();

    // Create a new Mosquitto client instance
    mosq = mosquitto_new(registration->wayru_device_id, true, NULL);
    if (!mosq) {
        console(CONSOLE_ERROR, "Error: Unable to create Mosquitto client instance.\n");
        mosquitto_lib_cleanup();
        return NULL;
    }

    pw_set = mosquitto_username_pw_set(mosq, mqtt_user, mqtt_password);
    if (pw_set != MOSQ_ERR_SUCCESS) {
        console(CONSOLE_ERROR, "Error: Unable to set username and password. %s\n", mosquitto_strerror(pw_set));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    char caPath[256];
    char keyPath[256];
    char crtPath[256];

    snprintf(caPath, sizeof(caPath), "%s/%s", config.data_path, CA_FILE_NAME);
    snprintf(keyPath, sizeof(keyPath), "%s/%s", config.data_path, KEY_FILE_NAME);
    snprintf(crtPath, sizeof(crtPath), "%s/%s", config.data_path, CERT_FILE_NAME);

    console(CONSOLE_DEBUG, "CA Path: %s", &caPath);
    console(CONSOLE_DEBUG, "Key Path: %s", &keyPath);
    console(CONSOLE_DEBUG, "Crt Path: %s", &crtPath);

    tls_set = mosquitto_tls_set(mosq, caPath, NULL, crtPath, keyPath, NULL);
    if (tls_set != MOSQ_ERR_SUCCESS) {
        console(CONSOLE_ERROR, "Error: Unable to set TLS. %s\n", mosquitto_strerror(tls_set));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    tls_opts_set = mosquitto_tls_opts_set(mosq, 1, "tlsv1.2", NULL);
    if (tls_opts_set != MOSQ_ERR_SUCCESS) {
        console(CONSOLE_ERROR, "Error: Unable to set TLS options. %s\n", mosquitto_strerror(tls_opts_set));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }
    // Set callbacks
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_publish_callback_set(mosq, on_publish);
    mosquitto_subscribe_callback_set(mosq, on_subscribe);

    // Connect to an MQTT broker
    rc = mosquitto_connect(mosq, config.mqtt_broker_url, 8883, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        console(CONSOLE_ERROR, "Error: Unable to connect to broker. %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    // Subscribe to a topic
    rc = mosquitto_subscribe(mosq, NULL, "test", 0);

    // Start the event loop
    rc = mosquitto_loop_start(mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
        console(CONSOLE_ERROR, "Error: Unable to start the event loop. %s\n", mosquitto_strerror(rc));
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    publish_mqtt(mosq, "wayru", "Hola desde una conexion segura!", 0);
    // Keep the program running to listen for messages
    // printf("Press Enter to exit...\n");
    // getchar();

    return mosq;
}

// @todo should probably exit the program if refresh fails
void refresh_mosquitto_access_token(struct mosquitto *mosq, AccessToken *access_token) {
    int pw_set = mosquitto_username_pw_set(mosq, access_token->token, "any");
    if (pw_set != MOSQ_ERR_SUCCESS) {
        console(CONSOLE_ERROR, "Unable to set username and password. %s\n", mosquitto_strerror(pw_set));
        return;
    }

    console(CONSOLE_INFO, "Mosquitto client access token refreshed.");
}

void clean_up_mosquitto(struct mosquitto **mosq) {
    mosquitto_disconnect(*mosq);

    for (int i = 0; i < topic_callbacks_count; i++) {
        if (topic_callbacks[i].topic) {
            free(topic_callbacks[i].topic);
            topic_callbacks[i].topic = NULL;
        }
    }

    topic_callbacks_count = 0;

    mosquitto_destroy(*mosq);
    mosquitto_lib_cleanup();
}

struct mosquitto *init_mqtt(Registration *registration, AccessToken *access_token) {
    return init_mosquitto(registration, access_token);
}
