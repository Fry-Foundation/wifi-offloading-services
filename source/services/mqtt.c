#include <stdio.h>
#include "mqtt.h"
#include <mosquitto.h>

void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
    printf("Connected to the broker.\n");

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


struct mosquitto * init_mosquitto() {
    struct mosquitto *mosq;
    int rc;

    // Initialize the Mosquitto library
    mosquitto_lib_init();

    // Create a new Mosquitto client instance
    mosq = mosquitto_new("client_id", true, NULL);
    if (!mosq) {
        fprintf(stderr, "Error: Unable to create Mosquitto client instance.\n");
        mosquitto_lib_cleanup();
        return 1;
    }

    // Set callbacks
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    // Connect to an MQTT broker
    rc = mosquitto_connect(mosq, "test.mosquitto.org", 1883, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error: Unable to connect to broker. %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    printf("Connected to the broker successfully.\n");

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

struct mosquitto * init_mqtt(){ 
    return init_mosquitto();
}