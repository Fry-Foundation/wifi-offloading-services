#include "mqtt.h"
#include "services/access_token.h"
#include "services/env.h"
#include "services/registration.h"
#include <lib/console.h>
#include <services/mqtt-cert.h>
#include <mosquitto.h>
#include <services/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOPIC_CALLBACKS 10

static Console csl = {
    .topic = "mqtt",
};

typedef struct {
    char *topic;
    MessageCallback callback;
} TopicCallback;

static TopicCallback topic_callbacks[MAX_TOPIC_CALLBACKS];
static int topic_callbacks_count = 0;

void on_connect(struct mosquitto *mosq, void *obj, int reason_code) {
    print_debug(&csl, "MQTT client on_connect callback, reason_code: %d", reason_code);

    if (reason_code) {
        print_error(&csl, "unable to connect to the broker. %s", mosquitto_connack_string(reason_code));
        // exit (1);
    } else {
        print_info(&csl, "connected to the broker.");
    }
}

void on_disconnect(struct mosquitto *mosq, void *obj, int reason_code) {
    print_info(&csl, "disconnected from the broker. Reason code: %d", reason_code);
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    for (int i = 0; i < topic_callbacks_count; i++) {
        if (strcmp(topic_callbacks[i].topic, msg->topic) == 0) {
            topic_callbacks[i].callback(mosq, msg);
        }
    }
}

void on_publish(struct mosquitto *mosq, void *obj, int mid) {
    print_info(&csl, "message has been published, mid %d", mid);
}

void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos, const int *granted_qos) {
    print_info(&csl, "subscribed to a topic, mid %d", mid);
}

void subscribe_mqtt(struct mosquitto *mosq, char *topic, int qos, MessageCallback callback) {
    if (topic_callbacks_count >= MAX_TOPIC_CALLBACKS) {
        print_error(&csl, "maximum number of topic callbacks reached.");
        return;
    }

    int rc = mosquitto_subscribe(mosq, NULL, topic, qos);
    if (rc != MOSQ_ERR_SUCCESS) {
        print_error(&csl, "unable to subscribe to the topic. %s", mosquitto_strerror(rc));
    } else {
        print_info(&csl, "subscribed to the topic %s successfully.", topic);
        topic_callbacks[topic_callbacks_count].topic = strdup(topic);
        topic_callbacks[topic_callbacks_count].callback = callback;
        topic_callbacks_count++;
    }
}

void publish_mqtt(struct mosquitto *mosq, char *topic, const char *message, int qos) {
    int rc = mosquitto_publish(mosq, NULL, topic, strlen(message), message, qos, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        print_error(&csl, "unable to publish message. %s\n", mosquitto_strerror(rc));
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
    print_debug(&csl, "user is %s", mqtt_user);
    print_debug(&csl, "password is %s", mqtt_password);

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
        print_error(&csl, "unable to create Mosquitto client instance.\n");
        mosquitto_lib_cleanup();
        return NULL;
    }

    pw_set = mosquitto_username_pw_set(mosq, mqtt_user, mqtt_password);
    if (pw_set != MOSQ_ERR_SUCCESS) {
        print_error(&csl, "unable to set username and password. %s\n", mosquitto_strerror(pw_set));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    char ca_path[256];
    char key_path[256];
    char crt_path[256];

    snprintf(ca_path, sizeof(ca_path), "%s/%s", config.data_path, MQTT_CA_FILE_NAME);
    snprintf(key_path, sizeof(key_path), "%s/%s", config.data_path, MQTT_KEY_FILE_NAME);
    snprintf(crt_path, sizeof(crt_path), "%s/%s", config.data_path, MQTT_CERT_FILE_NAME);

    print_debug(&csl, "CA Path: %s", &ca_path);
    print_debug(&csl, "Key Path: %s", &key_path);
    print_debug(&csl, "Crt Path: %s", &crt_path);

    tls_set = mosquitto_tls_set(mosq, ca_path, NULL, crt_path, key_path, NULL);
    if (tls_set != MOSQ_ERR_SUCCESS) {
        print_error(&csl, "unable to set TLS. %s\n", mosquitto_strerror(tls_set));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    tls_opts_set = mosquitto_tls_opts_set(mosq, 1, "tlsv1.2", NULL);
    if (tls_opts_set != MOSQ_ERR_SUCCESS) {
        print_error(&csl, "unable to set TLS options. %s\n", mosquitto_strerror(tls_opts_set));
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
        print_error(&csl, "unable to connect to broker. %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    // Subscribe to a topic
    rc = mosquitto_subscribe(mosq, NULL, "test", 0);

    // Start the event loop
    rc = mosquitto_loop_start(mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
        print_error(&csl, "unable to start the event loop. %s\n", mosquitto_strerror(rc));
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
        print_error(&csl, "Unable to set username and password. %s\n", mosquitto_strerror(pw_set));
        return;
    }

    print_info(&csl, "mosquitto client access token refreshed.");
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
