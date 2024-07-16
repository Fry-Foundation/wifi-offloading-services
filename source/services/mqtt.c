#include <stdio.h>
#include "mqtt.h"
#include "env.h"
#include <mosquitto.h>

void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
    if(reason_code){
        printf("connection error: %d (%s)\n", reason_code, mosquitto_connack_string(reason_code));
        //exit (1);
    }else {
        printf("Connected to the broker.\n");
    }

    int rc = mosquitto_subscribe(mosq, NULL, "test/topic", 0);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error: Unable to subscribe to the topic. %s\n", mosquitto_strerror(rc));
    } else {
        printf("Subscribed to the topic successfully.\n");
    }
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    printf("Received message: %s\n", (char *)msg->payload);
}


void on_publish(struct mosquitto *mosq, void *obj, int mid, int reason_code)
{
    printf("Message has been published.\n");
}

void publish_mqtt(struct mosquitto *mosq, char *topic, char *message) {
    int rc = mosquitto_publish(mosq, NULL, topic, strlen(message), message, 0, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error: Unable to publish message. %s\n", mosquitto_strerror(rc));
    }
}

struct mosquitto * init_mosquitto() {
    load_env("../.env");
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
        fprintf(stderr, "Error: Unable to create Mosquitto client instance.\n");
        mosquitto_lib_cleanup();
        return 1;
    }

    pw_set = mosquitto_username_pw_set(mosq, mqtt_user, mqtt_password);
    if(pw_set != MOSQ_ERR_SUCCESS){
        fprintf(stderr, "Error: Unable to set username and password. %s\n", mosquitto_strerror(pw_set));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    tls_set = mosquitto_tls_set(mosq, "../source/certificates/ca.crt", NULL, "../source/certificates/monitoring.crt", "../source/certificates/monitoring.key", NULL);
    if(tls_set != MOSQ_ERR_SUCCESS){
        fprintf(stderr, "Error: Unable to set TLS. %s\n", mosquitto_strerror(tls_set));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    tls_opts_set = mosquitto_tls_opts_set(mosq, 1, "tlsv1.2", NULL);
    if(tls_opts_set != MOSQ_ERR_SUCCESS){
        fprintf(stderr, "Error: Unable to set TLS options. %s\n", mosquitto_strerror(tls_opts_set));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }
    // Set callbacks
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_publish_callback_set(mosq, on_publish);

    // Connect to an MQTT broker
    rc = mosquitto_connect(mosq, "broker.internal.wayru.tech", 8883, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error: Unable to connect to broker. %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    // Subscribe to a topic
    rc = mosquitto_subscribe(mosq, NULL, "test", 0);

    // Start the event loop
    rc = mosquitto_loop_start(mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error: Unable to start the event loop. %s\n", mosquitto_strerror(rc));
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
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

struct mosquitto * init_mqtt() { 
    return init_mosquitto();
}