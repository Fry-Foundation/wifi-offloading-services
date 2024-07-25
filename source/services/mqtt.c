#include "mqtt.h"
#include "env.h"
#include <lib/console.h>
#include <mosquitto.h>
#include <services/config.h>
#include <stdio.h>
#include <string.h>

#define CA_FILE_NAME "ca.crt"
#define KEY_FILE_NAME "device.key"
#define CSR_FILE_NAME "device.csr"
#define CERT_FILE_NAME "device.crt"

void on_connect(struct mosquitto *mosq, void *obj, int reason_code) {
    if (reason_code) {
        console(CONSOLE_ERROR, "Error: Unable to connect to the broker. %s\n", mosquitto_connack_string(reason_code));
        // exit (1);
    } else {
        console(CONSOLE_INFO, "Connected to the broker.");
    }
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    console(CONSOLE_INFO, "Received message: %s\n", (char *)msg->payload);
}

void on_publish(struct mosquitto *mosq, void *obj, int mid) { console(CONSOLE_INFO, "Message has been published.\n"); }

void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos, const int *granted_qos) {
    console(CONSOLE_INFO, "Subscribed to a topic.\n");
}

void subscribe_mqtt(struct mosquitto *mosq, char *topic, int qos) {
    int rc = mosquitto_subscribe(mosq, NULL, topic, qos);
    if (rc != MOSQ_ERR_SUCCESS) {
        console(CONSOLE_ERROR, "Error: Unable to subscribe to the topic. %s\n", mosquitto_strerror(rc));
    } else {
        console(CONSOLE_INFO, "Subscribed to the topic successfully.");
    }
}

void publish_mqtt(struct mosquitto *mosq, char *topic, const char *message) {
    int rc = mosquitto_publish(mosq, NULL, topic, strlen(message), message, 0, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        console(CONSOLE_ERROR, "Error: Unable to publish message. %s\n", mosquitto_strerror(rc));
    }
}

struct mosquitto *init_mosquitto() {
    char env_file[256];
    snprintf(env_file, sizeof(env_file), "%s%s", config.data_path, "/.env");
    load_env(env_file);
    struct mosquitto *mosq;
    const char *mqtt_user = env("MQTT_USER");
    const char *mqtt_password = env("MQTT_PASS");
    int rc;
    int pw_set;
    int tls_set;
    int tls_opts_set;
    // Initialize the Mosquitto library
    mosquitto_lib_init();

    // Create a new Mosquitto client instance
    mosq = mosquitto_new("client_id", true, NULL);
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
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_publish_callback_set(mosq, on_publish);
    mosquitto_subscribe_callback_set(mosq, on_subscribe);

    // Connect to an MQTT broker
    rc = mosquitto_connect(mosq, "broker.internal.wayru.tech", 8883, 60);
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

    publish_mqtt(mosq, "wayru", "Hola desde una conexion segura!");
    // Keep the program running to listen for messages
    // printf("Press Enter to exit...\n");
    // getchar();

    return mosq;
}

void clean_up_mosquitto(struct mosquitto **mosq) {
    mosquitto_disconnect(*mosq);
    mosquitto_destroy(*mosq);
    mosquitto_lib_cleanup();
}

struct mosquitto *init_mqtt() {
    return init_mosquitto();
}