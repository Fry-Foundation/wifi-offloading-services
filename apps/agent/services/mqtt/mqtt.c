#include "mqtt.h"
#include "core/console.h"
#include "core/uloop_scheduler.h"
#include "services/diagnostic/diagnostic.h"
#include "services/exit_handler.h"
#include "services/mqtt/cert.h"
#include <errno.h>
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_TOPIC_CALLBACKS 10
#define CLEAN_SESSION true
#define TLS_VERIFY 1
#define TLS_VERSION "tlsv1.2"
#define PORT 8883
#define LAST_SUCCESSFUL_LOOP_TIMEOUT 300

// Error recovery limits and timeouts
#define MQTT_INVALID_PARAM_MAX_ATTEMPTS 3
#define MQTT_MEMORY_ERROR_MAX_ATTEMPTS 2
#define MQTT_UNKNOWN_ERROR_MAX_ATTEMPTS 3
#define MQTT_MEMORY_ERROR_DELAY_SECONDS 5
#define MQTT_RECONNECT_MAX_ATTEMPTS 5
#define MQTT_RECONNECT_BASE_DELAY_SECONDS 30
#define MQTT_RECONNECT_MAX_DELAY_SECONDS 150
#define MQTT_CONNECTION_STABILIZE_DELAY_SECONDS 1

static Console csl = {
    .topic = "mqtt",
};

struct MqttTaskContext {
    Mosq *mosq;
    MqttConfig config;
    int invalid_state_count;
    int protocol_error_count;
    int memory_error_count;
    int unknown_error_count;
    task_id_t task_id; // Store current task ID for rescheduling
};

typedef struct {
    char *topic;
    int qos;
    MessageCallback callback;
} TopicCallback;

static TopicCallback topic_callbacks[MAX_TOPIC_CALLBACKS];
static int topic_callbacks_count = 0;

void on_connect(Mosq *mosq, void *obj, int reason_code) {
    console_debug(&csl, "MQTT client on_connect callback, reason_code: %d", reason_code);

    if (reason_code) {
        console_error(&csl, "unable to connect to the broker. %s", mosquitto_connack_string(reason_code));
    } else {
        console_info(&csl, "connected to the broker");
    }
}

void on_disconnect(Mosq *mosq, void *obj, int reason_code) {
    console_info(&csl, "Disconnected from broker, reason_code: %d", reason_code);

    if (reason_code == 0) {
        console_info(&csl, "Normal disconnection");
    } else {
        console_error(&csl, "Unexpected disconnection: %s", mosquitto_reason_string(reason_code));
    }

    // Note: Don't attempt reconnection here - let the main loop handle it
    // This prevents potential race conditions and duplicate reconnection attempts
}

void on_message(Mosq *mosq, void *obj, const struct mosquitto_message *msg) {
    for (int i = 0; i < topic_callbacks_count; i++) {
        if (strcmp(topic_callbacks[i].topic, msg->topic) == 0) {
            topic_callbacks[i].callback(mosq, msg);
        }
    }
}

void on_publish(Mosq *mosq, void *obj, int mid) {
    console_info(&csl, "message has been published, message id %d", mid);
}

void on_subscribe(Mosq *mosq, void *obj, int mid, int qos, const int *granted_qos) {
    console_info(&csl, "subscribed to a topic, message id %d", mid);
}

void subscribe_mqtt(Mosq *mosq, char *topic, int qos, MessageCallback callback) {
    if (topic_callbacks_count >= MAX_TOPIC_CALLBACKS) {
        console_error(&csl, "maximum number of topic callbacks reached");
        return;
    }

    int rc = mosquitto_subscribe(mosq, NULL, topic, qos);
    if (rc != MOSQ_ERR_SUCCESS) {
        console_error(&csl, "unable to subscribe to the topic '%s'", mosquitto_strerror(rc));
    } else {
        console_info(&csl, "subscribed to the topic %s successfully", topic);
        topic_callbacks[topic_callbacks_count].topic = strdup(topic);
        topic_callbacks[topic_callbacks_count].callback = callback;
        topic_callbacks[topic_callbacks_count].qos = qos;
        topic_callbacks_count++;
    }
}

void publish_mqtt(Mosq *mosq, char *topic, const char *message, int qos) {
    int rc = mosquitto_publish(mosq, NULL, topic, strlen(message), message, qos, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        console_error(&csl, "unable to publish message. %s", mosquitto_strerror(rc));
    }
}

Mosq *init_mosquitto(const MqttConfig *config) {
    console_debug(&csl, "user is %s", config->username);
    console_debug(&csl, "password is %s", config->password);

    Mosq *mosq;
    int rc;
    // Initialize the Mosquitto library
    mosquitto_lib_init();

    // Create a new Mosquitto client instance
    mosq = mosquitto_new(config->client_id, CLEAN_SESSION, NULL);
    if (!mosq) {
        console_error(&csl, "unable to create Mosquitto client instance.\n");
        mosquitto_lib_cleanup();
        return NULL;
    }

    int pw_set = mosquitto_username_pw_set(mosq, config->username, config->password);
    if (pw_set != MOSQ_ERR_SUCCESS) {
        console_error(&csl, "unable to set username and password. %s\n", mosquitto_strerror(pw_set));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    char ca_path[296];
    char key_path[296];
    char crt_path[296];

    snprintf(ca_path, sizeof(ca_path), "%s/%s", config->data_path, MQTT_CA_FILE_NAME);
    snprintf(key_path, sizeof(key_path), "%s/%s", config->data_path, MQTT_KEY_FILE_NAME);
    snprintf(crt_path, sizeof(crt_path), "%s/%s", config->data_path, MQTT_CERT_FILE_NAME);

    console_debug(&csl, "CA Path: %s", ca_path);
    console_debug(&csl, "Key Path: %s", key_path);
    console_debug(&csl, "Crt Path: %s", crt_path);

    int tls_set = mosquitto_tls_set(mosq, ca_path, NULL, crt_path, key_path, NULL);
    if (tls_set != MOSQ_ERR_SUCCESS) {
        console_error(&csl, "unable to set TLS. %s\n", mosquitto_strerror(tls_set));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    int tls_opts_set = mosquitto_tls_opts_set(mosq, TLS_VERIFY, TLS_VERSION, NULL);
    if (tls_opts_set != MOSQ_ERR_SUCCESS) {
        console_error(&csl, "unable to set TLS options. %s\n", mosquitto_strerror(tls_opts_set));
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
    rc = mosquitto_connect(mosq, config->broker_url, PORT, config->keepalive);
    if (rc != MOSQ_ERR_SUCCESS) {
        console_error(&csl, "unable to connect to broker. %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return NULL;
    }

    return mosq;
}

void resubscribe_mqtt(Mosq *mosq) {
    console_info(&csl, "Resuscribing to %d topics", topic_callbacks_count);
    for (int i = 0; i < topic_callbacks_count; i++) {
        int rc = mosquitto_subscribe(mosq, NULL, topic_callbacks[i].topic, topic_callbacks[i].qos);
        if (rc != MOSQ_ERR_SUCCESS) {
            console_error(&csl, "unable to resubscribe to the topic '%s'. %s", topic_callbacks[i].topic,
                          mosquitto_strerror(rc));
        } else {
            console_info(&csl, "resubscribed to the topic %s successfully", topic_callbacks[i].topic);
        }
    }
}

void refresh_mosquitto_credentials(Mosq *mosq, const char *username) {
    int pw_set = mosquitto_username_pw_set(mosq, username, "any");
    if (pw_set != MOSQ_ERR_SUCCESS) {
        console_error(&csl, "Unable to set username and password. %s\n", mosquitto_strerror(pw_set));
        return;
    }

    console_info(&csl, "mosquitto client credentials refreshed.");
}

// Enhanced function that handles both lightweight reconnection and full reinitialization
// force_full_reinit: if true, skip lightweight reconnection and go straight to full reinitialization
static bool mqtt_recover(MqttTaskContext *context, bool force_full_reinit) {
    static int reconnect_attempt = 0;

    while (reconnect_attempt < MQTT_RECONNECT_MAX_ATTEMPTS) {
        reconnect_attempt++;

        // Exponential backoff with jitter
        int delay = MQTT_RECONNECT_BASE_DELAY_SECONDS * (1 << (reconnect_attempt - 1));
        if (delay > MQTT_RECONNECT_MAX_DELAY_SECONDS) delay = MQTT_RECONNECT_MAX_DELAY_SECONDS;

        console_info(&csl, "Attempting reconnection (attempt %d/%d) in %d seconds", reconnect_attempt,
                     MQTT_RECONNECT_MAX_ATTEMPTS, delay);
        sleep(delay);

        // Strategy 1: Try lightweight reconnect first (unless forced to skip)
        if (!force_full_reinit) {
            console_info(&csl, "Trying lightweight reconnection...");
            int rc = mosquitto_reconnect(context->mosq);
            if (rc == MOSQ_ERR_SUCCESS) {
                console_info(&csl, "Lightweight reconnection successful");

                // Wait for connection to stabilize
                sleep(MQTT_CONNECTION_STABILIZE_DELAY_SECONDS);

                // Resubscribe to all topics
                resubscribe_mqtt(context->mosq);

                // Reset attempt counter on success
                reconnect_attempt = 0;
                update_led_status(true, "MQTT reconnected");
                return true;
            } else {
                console_error(&csl, "Lightweight reconnection failed: %s", mosquitto_strerror(rc));
            }
        } else {
            console_info(&csl, "Skipping lightweight reconnection due to error type requiring full reinitialization");
        }

        // Strategy 2: Try complete reinitialization as fallback
        console_info(&csl, "Trying complete reinitialization...");

        // Reinitialize the client
        int rc = mosquitto_reinitialise(context->mosq, context->config.client_id, CLEAN_SESSION, NULL);
        if (rc != MOSQ_ERR_SUCCESS) {
            console_error(&csl, "Client reinitialization failed: %s", mosquitto_strerror(rc));
            continue; // Try next attempt
        }

        // Reconfigure credentials
        int pw_set = mosquitto_username_pw_set(context->mosq, context->config.username, context->config.password);
        if (pw_set != MOSQ_ERR_SUCCESS) {
            console_error(&csl, "Failed to set credentials: %s", mosquitto_strerror(pw_set));
            continue; // Try next attempt
        }

        // Reconfigure TLS
        char ca_path[296];
        char key_path[296];
        char crt_path[296];

        snprintf(ca_path, sizeof(ca_path), "%s/%s", context->config.data_path, MQTT_CA_FILE_NAME);
        snprintf(key_path, sizeof(key_path), "%s/%s", context->config.data_path, MQTT_KEY_FILE_NAME);
        snprintf(crt_path, sizeof(crt_path), "%s/%s", context->config.data_path, MQTT_CERT_FILE_NAME);

        int tls_set = mosquitto_tls_set(context->mosq, ca_path, NULL, crt_path, key_path, NULL);
        if (tls_set != MOSQ_ERR_SUCCESS) {
            console_error(&csl, "Failed to set TLS: %s", mosquitto_strerror(tls_set));
            continue; // Try next attempt
        }

        int tls_opts_set = mosquitto_tls_opts_set(context->mosq, TLS_VERIFY, TLS_VERSION, NULL);
        if (tls_opts_set != MOSQ_ERR_SUCCESS) {
            console_error(&csl, "Failed to set TLS options: %s", mosquitto_strerror(tls_opts_set));
            continue; // Try next attempt
        }

        // Reset callbacks
        mosquitto_connect_callback_set(context->mosq, on_connect);
        mosquitto_disconnect_callback_set(context->mosq, on_disconnect);
        mosquitto_message_callback_set(context->mosq, on_message);
        mosquitto_publish_callback_set(context->mosq, on_publish);
        mosquitto_subscribe_callback_set(context->mosq, on_subscribe);

        // Attempt new connection
        rc = mosquitto_connect(context->mosq, context->config.broker_url, PORT, context->config.keepalive);
        if (rc == MOSQ_ERR_SUCCESS) {
            console_info(&csl, "Complete reinitialization successful");

            // Wait for connection to stabilize
            sleep(MQTT_CONNECTION_STABILIZE_DELAY_SECONDS);

            // Resubscribe to all topics
            resubscribe_mqtt(context->mosq);

            // Reset attempt counter on success
            reconnect_attempt = 0;
            update_led_status(true, "MQTT fully reinitialized");
            return true;
        } else {
            console_error(&csl, "Complete reinitialization failed: %s", mosquitto_strerror(rc));
        }
    }

    // If we get here, all reconnection attempts failed
    console_error(&csl, "All reconnection strategies failed, requesting exit");
    update_led_status(false, "MQTT recovery failed");
    request_cleanup_and_exit("MQTT reconnection failed after all attempts");
    return false;
}

void mqtt_task(void *task_context) {
    MqttTaskContext *context = (MqttTaskContext *)task_context;

    // Check if shutdown has been requested by another component
    if (is_shutdown_requested()) {
        console_info(&csl, "Shutdown requested, stopping MQTT task");
        return;
    }

    console_info(&csl, "running mqtt task");
    int res = mosquitto_loop(context->mosq, -1, 1);

    bool should_reschedule = true;
    static time_t last_successful_loop = 0;

    /*
     * Error Recovery Strategy:
     * - Connection errors (NO_CONN, CONN_LOST): Try lightweight reconnect first
     * - State corruption risks (PROTOCOL, INVAL, NOMEM, ERRNO): Force full reinitialization
     * - Unknown errors: Try lightweight reconnect first (might be transient)
     * - Extended failures: Force full reinitialization after timeout
     *
     * Rationale: Some errors can leave "ghost errors" in the mosquitto client state
     * that persist even after successful reconnection. Full reinitialization ensures
     * a clean slate for these problematic error types.
     */
    switch (res) {
    case MOSQ_ERR_SUCCESS:
        console_info(&csl, "mosquitto loop success");
        // Reset all error counters on success
        context->invalid_state_count = 0;
        context->protocol_error_count = 0;
        context->memory_error_count = 0;
        context->unknown_error_count = 0;
        last_successful_loop = time(NULL);
        update_led_status(true, "MQTT successful");
        break;

    case MOSQ_ERR_NO_CONN:
        console_error(&csl, "MQTT error: No connection to broker");
        if (!mqtt_recover(context, false)) {
            should_reschedule = false;
        }
        break;

    case MOSQ_ERR_CONN_LOST:
        console_error(&csl, "MQTT error: Connection to broker lost");
        if (!mqtt_recover(context, false)) {
            should_reschedule = false;
        }
        break;

    case MOSQ_ERR_ERRNO: {
        char error_buf[256];
        strerror_r(errno, error_buf, sizeof(error_buf));
        console_error(&csl, "MQTT error: System error occurred (errno: %d, %s)", errno, error_buf);
    }
        // System call failures can corrupt internal client state - force full reinitialization
        if (!mqtt_recover(context, true)) {
            should_reschedule = false;
        }
        break;

    case MOSQ_ERR_PROTOCOL:
        console_error(&csl, "MQTT error: Protocol error communicating with broker");
        // Protocol errors are serious - force immediate full reinitialization
        if (!mqtt_recover(context, true)) {
            should_reschedule = false;
        }
        break;

    case MOSQ_ERR_INVAL:
        console_error(&csl, "MQTT error: Invalid parameters");
        context->invalid_state_count++;

        if (context->invalid_state_count <= MQTT_INVALID_PARAM_MAX_ATTEMPTS) {
            console_info(
                &csl, "Invalid parameter error count: %d/%d, forcing full recovery due to potential state corruption",
                context->invalid_state_count, MQTT_INVALID_PARAM_MAX_ATTEMPTS);
        }

        // Force full reinitialization for invalid parameters as they may indicate corrupted state
        if (!mqtt_recover(context, true)) {
            should_reschedule = false;
        } else {
            context->invalid_state_count = 0; // Reset on success
        }
        break;

    case MOSQ_ERR_NOMEM:
        console_error(&csl, "MQTT error: Out of memory");
        context->memory_error_count++;

        if (context->memory_error_count <= MQTT_MEMORY_ERROR_MAX_ATTEMPTS) {
            console_info(&csl, "Memory error count: %d/%d, waiting %d seconds before full recovery",
                         context->memory_error_count, MQTT_MEMORY_ERROR_MAX_ATTEMPTS, MQTT_MEMORY_ERROR_DELAY_SECONDS);
            // For memory errors, wait a bit longer before attempting recovery
            sleep(MQTT_MEMORY_ERROR_DELAY_SECONDS);
        }

        // Force full reinitialization for memory errors as they may leave inconsistent state
        if (!mqtt_recover(context, true)) {
            should_reschedule = false;
        } else {
            context->memory_error_count = 0; // Reset on success
        }
        break;

    default:
        console_error(&csl, "MQTT error: Unknown error code %d (%s)", res, mosquitto_strerror(res));
        context->unknown_error_count++;

        if (context->unknown_error_count <= MQTT_UNKNOWN_ERROR_MAX_ATTEMPTS) {
            console_info(&csl, "Unknown error count: %d/%d, attempting full recovery", context->unknown_error_count,
                         MQTT_UNKNOWN_ERROR_MAX_ATTEMPTS);
        }

        if (!mqtt_recover(context, false)) {
            should_reschedule = false;
        } else {
            context->unknown_error_count = 0; // Reset on success
        }
        break;
    }

    // Connection health monitoring
    if (last_successful_loop > 0 && time(NULL) - last_successful_loop > LAST_SUCCESSFUL_LOOP_TIMEOUT) {
        console_error(&csl, "No successful MQTT operations for %d seconds, forcing reconnection",
                      LAST_SUCCESSFUL_LOOP_TIMEOUT);
        // Force full reinitialization after extended period of failures to clear any accumulated issues
        if (!mqtt_recover(context, true)) {
            should_reschedule = false;
        }
    }

    // Schedule the next task execution
    if (should_reschedule) {
        uint32_t interval_ms = context->config.task_interval * 1000;
        console_debug(&csl, "Rescheduling MQTT task in %u ms", interval_ms);
        context->task_id = schedule_once(interval_ms, mqtt_task, context);
        if (context->task_id == 0) {
            console_error(&csl, "Failed to reschedule MQTT task");
        }
    }
}

MqttTaskContext *mqtt_service(Mosq *mosq, const MqttConfig *config) {
    MqttTaskContext *context = (MqttTaskContext *)malloc(sizeof(MqttTaskContext));
    if (context == NULL) {
        console_error(&csl, "failed to allocate memory for mqtt task context");
        return NULL;
    }

    context->mosq = mosq;
    context->config = *config;
    context->invalid_state_count = 0;
    context->protocol_error_count = 0;
    context->memory_error_count = 0;
    context->unknown_error_count = 0;
    context->task_id = 0;

    // Schedule immediate execution
    console_info(&csl, "Starting MQTT service");
    context->task_id = schedule_once(0, mqtt_task, context);

    if (context->task_id == 0) {
        console_error(&csl, "Failed to schedule MQTT task");
        free(context);
        return NULL;
    }

    console_debug(&csl, "Successfully scheduled MQTT task with ID %u", context->task_id);
    return context;
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
    console_info(&csl, "cleaned mqtt client");
}

// MQTT access token refresh callback function
void mqtt_token_refresh_callback(const char *new_token, void *context) {
    MqttClient *client = (MqttClient *)context;
    // Update the MQTT client with the new token
    refresh_mosquitto_credentials(client->mosq, new_token);
    // Update the config struct
    client->config.username = new_token;
}

void clean_mqtt_context(MqttTaskContext *context) {
    console_debug(&csl, "clean_mqtt_context called with context: %p", context);
    if (context != NULL) {
        if (context->task_id != 0) {
            console_debug(&csl, "Cancelling MQTT task %u", context->task_id);
            cancel_task(context->task_id);
        }
        console_debug(&csl, "Freeing MQTT context %p", context);
        free(context);
    }
}

// Function to create MQTT access token refresh callback
AccessTokenCallbacks create_mqtt_token_callbacks(MqttClient *client) {
    AccessTokenCallbacks callbacks = {.on_token_refresh = mqtt_token_refresh_callback, .context = client};
    return callbacks;
}

Mosq *init_mqtt(const MqttConfig *config) { return init_mosquitto(config); }
