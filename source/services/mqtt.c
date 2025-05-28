#include "mqtt.h"
#include "services/access_token.h"
#include "services/diagnostic.h"
#include "services/env.h"
#include "services/exit_handler.h"
#include "services/registration.h"
#include <lib/console.h>
#include <mosquitto.h>
#include <services/config.h>
#include <services/mqtt-cert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_TOPIC_CALLBACKS 10
#define CLEAN_SESSION true
#define TLS_VERIFY 1
#define TLS_VERSION "tlsv1.2"
#define PORT 8883
#define KEEP_ALIVE config.mqtt_keepalive
#define TASK_INTERVAL config.mqtt_task_interval

static Console csl = {
    .topic = "mqtt",
};

typedef struct {
    Mosq *mosq;
    Registration *registration;
    AccessToken *access_token;
    int default_attempt_count;
} MqttTaskContext;

typedef struct {
    char *topic;
    int qos;
    MessageCallback callback;
} TopicCallback;

static TopicCallback topic_callbacks[MAX_TOPIC_CALLBACKS];
static int topic_callbacks_count = 0;

void on_connect(struct mosquitto *mosq, void *obj, int reason_code) {
    print_debug(&csl, "MQTT client on_connect callback, reason_code: %d", reason_code);

    if (reason_code) {
        print_error(&csl, "unable to connect to the broker. %s", mosquitto_connack_string(reason_code));
    } else {
        print_info(&csl, "connected to the broker");
    }
}

void on_disconnect(struct mosquitto *mosq, void *obj, int reason_code) {
    print_debug(&csl, "MQTT client on_disconnect callback, reason_code: %d", reason_code);
    print_debug(&csl, "disconnecting from the broker");
}

void reconnect(struct mosquitto *mosq, void *obj, int reason_code) {
    static int retry_count = 0;
    const int max_retries = 3;
    const int initial_retry_delay = 5; // 5 seconds

    // Log the disconnection reason
    if (reason_code == 0) {
        print_info(&csl, "disconnected from the broker");
    } else {
        print_error(&csl, "unexpectedly disconnected from the broker");
        print_error(&csl, "reason code: %d", reason_code);
        print_error(&csl, "reason string: %s", mosquitto_reason_string(reason_code));
    }

    // Attempt to reconnect if the disconnection was unexpected
    while (reason_code != 0 && retry_count < max_retries) {
        retry_count++;
        int delay = initial_retry_delay * (1 << (retry_count - 1)); // Exponential backoff
        print_info(&csl, "reconnecting in %d seconds (attempt %d/%d)...", delay, retry_count, max_retries);
        sleep(delay);

        int rc = mosquitto_reconnect(mosq);
        if (rc == MOSQ_ERR_SUCCESS) {
            print_info(&csl, "reconnected successfully.");
            retry_count = 0; // Reset retry count on success
            break;
        } else {
            print_error(&csl, "reconnection attempt failed; error code is %d", rc);
        }
    }

    if (retry_count >= max_retries) {
        print_error(&csl, "maximum reconnection attempts reached. Giving up and exiting ...");
        // clean_up_mosquitto(&mosq);
        // cleanup_and_exit(1);
        update_led_status(false, "MQTT check");
        request_cleanup_and_exit();
    }

    update_led_status(true, "MQTT check");
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    for (int i = 0; i < topic_callbacks_count; i++) {
        if (strcmp(topic_callbacks[i].topic, msg->topic) == 0) {
            topic_callbacks[i].callback(mosq, msg);
        }
    }
}

void on_publish(struct mosquitto *mosq, void *obj, int mid) {
    print_info(&csl, "message has been published, message id %d", mid);
}

void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos, const int *granted_qos) {
    print_info(&csl, "subscribed to a topic, message id %d", mid);
}

void subscribe_mqtt(struct mosquitto *mosq, char *topic, int qos, MessageCallback callback) {
    if (topic_callbacks_count >= MAX_TOPIC_CALLBACKS) {
        print_error(&csl, "maximum number of topic callbacks reached");
        return;
    }

    int rc = mosquitto_subscribe(mosq, NULL, topic, qos);
    if (rc != MOSQ_ERR_SUCCESS) {
        print_error(&csl, "unable to subscribe to the topic '%s'", mosquitto_strerror(rc));
    } else {
        print_info(&csl, "subscribed to the topic %s successfully", topic);
        topic_callbacks[topic_callbacks_count].topic = strdup(topic);
        topic_callbacks[topic_callbacks_count].callback = callback;
        topic_callbacks[topic_callbacks_count].qos = qos;
        topic_callbacks_count++;
    }
}

void publish_mqtt(struct mosquitto *mosq, char *topic, const char *message, int qos) {
    int rc = mosquitto_publish(mosq, NULL, topic, strlen(message), message, qos, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        print_error(&csl, "unable to publish message. %s", mosquitto_strerror(rc));
    }
}

struct mosquitto *init_mosquitto(Registration *registration, AccessToken *access_token) {
    // Load user and password from env file
    // If present, these override the access token
    char env_file[270];
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
    // Initialize the Mosquitto library
    mosquitto_lib_init();

    // Create a new Mosquitto client instance
    mosq = mosquitto_new(registration->wayru_device_id, CLEAN_SESSION, NULL);
    if (!mosq) {
        print_error(&csl, "unable to create Mosquitto client instance.\n");
        mosquitto_lib_cleanup();
        return NULL;
    }

    int pw_set = mosquitto_username_pw_set(mosq, mqtt_user, mqtt_password);
    if (pw_set != MOSQ_ERR_SUCCESS) {
        print_error(&csl, "unable to set username and password. %s\n", mosquitto_strerror(pw_set));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    char ca_path[296];
    char key_path[296];
    char crt_path[296];

    snprintf(ca_path, sizeof(ca_path), "%s/%s", config.data_path, MQTT_CA_FILE_NAME);
    snprintf(key_path, sizeof(key_path), "%s/%s", config.data_path, MQTT_KEY_FILE_NAME);
    snprintf(crt_path, sizeof(crt_path), "%s/%s", config.data_path, MQTT_CERT_FILE_NAME);

    print_debug(&csl, "CA Path: %s", &ca_path);
    print_debug(&csl, "Key Path: %s", &key_path);
    print_debug(&csl, "Crt Path: %s", &crt_path);

    int tls_set = mosquitto_tls_set(mosq, ca_path, NULL, crt_path, key_path, NULL);
    if (tls_set != MOSQ_ERR_SUCCESS) {
        print_error(&csl, "unable to set TLS. %s\n", mosquitto_strerror(tls_set));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    int tls_opts_set = mosquitto_tls_opts_set(mosq, TLS_VERIFY, TLS_VERSION, NULL);
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
    rc = mosquitto_connect(mosq, config.mqtt_broker_url, PORT, KEEP_ALIVE);
    if (rc != MOSQ_ERR_SUCCESS) {
        print_error(&csl, "unable to connect to broker. %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    // Start the event loop (threaded version, but works fine on single core devices like the Genesis)
    // rc = mosquitto_loop_start(mosq);
    // if (rc != MOSQ_ERR_SUCCESS) {
    //     print_error(&csl, "unable to start the event loop. %s\n", mosquitto_strerror(rc));
    //     mosquitto_disconnect(mosq);
    //     mosquitto_destroy(mosq);
    //     mosquitto_lib_cleanup();
    //     return NULL;
    // }

    return mosq;
}

void resubscribe_mqtt(Mosq *mosq) {
    print_info(&csl, "Resuscribing to %d topics", topic_callbacks_count);
    for (int i = 0; i < topic_callbacks_count; i++) {
        int rc = mosquitto_subscribe(mosq, NULL, topic_callbacks[i].topic, topic_callbacks[i].qos);
        if (rc != MOSQ_ERR_SUCCESS) {
            print_error(&csl, "unable to resubscribe to the topic '%s'. %s", topic_callbacks[i].topic,
                        mosquitto_strerror(rc));
        } else {
            print_info(&csl, "resubscribed to the topic %s successfully", topic_callbacks[i].topic);
        }
    }
}

struct mosquitto *reinit_mosquitto(Mosq *mosq, Registration *registration, AccessToken *access_token) {

    const char *mqtt_user = access_token->token;
    const char *mqtt_password = "any";
    // Reinitialize the Mosquitto client instance
    int rc = mosquitto_reinitialise(mosq, registration->wayru_device_id, CLEAN_SESSION, NULL);
    if (rc != MOSQ_ERR_SUCCESS) {
        print_error(&csl, "unable to reinitialise Mosquitto client instance. %s\n", mosquitto_strerror(rc));
        cleanup_mqtt(&mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    if (!mosq) {
        print_error(&csl, "unable to reinitialise Mosquitto client instance.\n");
        cleanup_mqtt(&mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    int pw_set = mosquitto_username_pw_set(mosq, mqtt_user, mqtt_password);
    if (pw_set != MOSQ_ERR_SUCCESS) {
        print_error(&csl, "unable to set username and password. %s\n", mosquitto_strerror(pw_set));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    char ca_path[280];
    char key_path[280];
    char crt_path[280];

    snprintf(ca_path, sizeof(ca_path), "%s/%s", config.data_path, MQTT_CA_FILE_NAME);
    snprintf(key_path, sizeof(key_path), "%s/%s", config.data_path, MQTT_KEY_FILE_NAME);
    snprintf(crt_path, sizeof(crt_path), "%s/%s", config.data_path, MQTT_CERT_FILE_NAME);

    int tls_set = mosquitto_tls_set(mosq, ca_path, NULL, crt_path, key_path, NULL);
    if (tls_set != MOSQ_ERR_SUCCESS) {
        print_error(&csl, "unable to set TLS. %s\n", mosquitto_strerror(tls_set));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    int tls_opts_set = mosquitto_tls_opts_set(mosq, TLS_VERIFY, TLS_VERSION, NULL);
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
    rc = mosquitto_connect(mosq, config.mqtt_broker_url, PORT, KEEP_ALIVE);
    if (rc != MOSQ_ERR_SUCCESS) {
        print_error(&csl, "unable to connect to broker. %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

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

void mqtt_task(Scheduler *sch, void *task_context) {
    MqttTaskContext *context = (MqttTaskContext *)task_context;
    print_info(&csl, "running mqtt task");
    int res = mosquitto_loop(context->mosq, -1, 1);
    switch (res) {
    case MOSQ_ERR_SUCCESS:
        print_info(&csl, "mosquitto loop success");
        if (context->default_attempt_count > 0) {
            context->default_attempt_count = 0;
        }
        break;
    case MOSQ_ERR_INVAL:
        print_error(&csl, "mosquitto loop invalid parameters");
        break;
    case MOSQ_ERR_NOMEM:
        print_error(&csl, "mosquitto loop out of memory");
        break;
    case MOSQ_ERR_NO_CONN:
        print_error(&csl, "mosquitto loop no connection");
        reconnect(context->mosq, context, res);
        break;
    case MOSQ_ERR_CONN_LOST:
        print_error(&csl, "mosquitto loop connection lost");
        reconnect(context->mosq, context, res);
        break;
    case MOSQ_ERR_PROTOCOL:
        print_error(&csl, "mosquitto loop protocol error");
        break;
    case MOSQ_ERR_ERRNO:
        print_error(&csl, "mosquitto loop error");
        reconnect(context->mosq, context, res);
        break;
    default:
        // Renitialize the mosquitto client when the result is unknown
        // (usually due to a session timeout or any unkown response code)
        print_error(&csl, "mosquitto loop unknown result");
        print_info(&csl, "reinitializing mosquitto client");
        // Reinitialize the mosquitto client
        context->mosq = reinit_mosquitto(context->mosq, context->registration, context->access_token);
        if (context->mosq == NULL) {
            // If mosq is NULL then initialize the mosquitto client again
            print_error(&csl, "failed to reinitialize mosquitto client");
            context->mosq = init_mosquitto(context->registration, context->access_token);
        }
        // Increment the default attempt count
        context->default_attempt_count++;
        if (context->default_attempt_count > 3) {
            print_error(&csl, "exceeded maximum retries for unknown result");
            cleanup_and_exit(1);
        }
        print_info(&csl, "mosquitto client reinitialized successfully");
        // Refresh the mosquitto client access token
        refresh_mosquitto_access_token(context->mosq, context->access_token);
        // Resubscribe to the topics
        resubscribe_mqtt(context->mosq);
        // Re-run the mqtt task
        mqtt_task(sch, context);
        return;
    }
    schedule_task(sch, time(NULL) + TASK_INTERVAL, mqtt_task, "mqtt task", context);
}

void mqtt_service(Scheduler *sch, Mosq *mosq, Registration *registration, AccessToken *access_token) {
    MqttTaskContext *context = (MqttTaskContext *)malloc(sizeof(MqttTaskContext));
    if (context == NULL) {
        print_error(&csl, "failed to allocate memory for mqtt task context");
        cleanup_and_exit(1);
        return;
    }

    context->mosq = mosq;
    context->registration = registration;
    context->access_token = access_token;
    context->default_attempt_count = 0;

    schedule_task(sch, time(NULL), mqtt_task, "mqtt task", context);
}

void cleanup_mqtt(Mosq **mosq) {
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
    print_info(&csl, "cleaned mqtt client");
}

struct mosquitto *init_mqtt(Registration *registration, AccessToken *access_token) {
    return init_mosquitto(registration, access_token);
}
