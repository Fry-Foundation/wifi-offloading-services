// For strdup on some systems
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "websocket.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

// Error recovery limits and timeouts (similar to MQTT implementation)
#define WS_NETWORK_ERROR_MAX_ATTEMPTS 5
#define WS_PROTOCOL_ERROR_MAX_ATTEMPTS 3
#define WS_MEMORY_ERROR_MAX_ATTEMPTS 2
#define WS_TIMEOUT_ERROR_MAX_ATTEMPTS 4
#define WS_TLS_ERROR_MAX_ATTEMPTS 3
#define WS_UNKNOWN_ERROR_MAX_ATTEMPTS 3
#define WS_MEMORY_ERROR_DELAY_SECONDS 5.0
#define WS_CONNECTION_STABILIZE_DELAY_SECONDS 1.0
#define WS_CONNECTION_TIMEOUT_THRESHOLD 300.0  // 5 minutes without successful ops

// Internal helper functions
static void set_error(ws_client_t *client, ws_error_type_t type, int curl_code, const char *message);
static ws_msg_type_t frame_type_to_msg_type(int flags);
static int msg_type_to_curl_flags(ws_msg_type_t type);
static ws_error_type_t curl_code_to_error_type(CURLcode code);
static double get_current_time(void);
static double calculate_reconnect_delay(ws_client_t *client);
static bool should_attempt_reconnect(ws_client_t *client);
static void reset_health_check(ws_client_t *client);
static void update_health_check(ws_client_t *client, ws_msg_type_t msg_type);
static void increment_error_counter(ws_client_t *client, ws_error_type_t error_type);
static bool ws_client_recover(ws_client_t *client, bool force_full_reconnect);
static void record_successful_operation(ws_client_t *client);
static void init_tls_config(ws_tls_config_t *tls);
static void free_tls_config(ws_tls_config_t *tls);
static void apply_tls_config(CURL *curl, const ws_tls_config_t *tls);

ws_client_t* ws_client_new(const char *url) {
    if (!url) return NULL;
    
    ws_client_t *client = calloc(1, sizeof(ws_client_t));
    if (!client) return NULL;
    
    client->url = strdup(url);
    if (!client->url) {
        free(client);
        return NULL;
    }
    
    client->curl = curl_easy_init();
    if (!client->curl) {
        free(client->url);
        free(client);
        return NULL;
    }
    
    client->state = WS_STATE_DISCONNECTED;
    client->connect_timeout = 30L;  // 30 seconds default
    client->read_timeout = 10L;     // 10 seconds default
    
    // Initialize TLS configuration with defaults
    init_tls_config(&client->tls);
    
    // Initialize reconnection config with defaults
    client->reconnect.enabled = false;
    client->reconnect.max_attempts = 5;
    client->reconnect.current_attempt = 0;
    client->reconnect.initial_interval = 1.0;  // 1 second
    client->reconnect.max_interval = 30.0;     // 30 seconds
    client->reconnect.backoff_multiplier = 2.0;
    client->reconnect.jitter_factor = 0.1;     // 10% jitter
    client->reconnect.last_attempt_time = 0;
    
    // Initialize health monitoring
    client->ping_interval = 30.0;  // 30 seconds
    client->pong_timeout = 10.0;   // 10 seconds
    client->connection_timeout_threshold = WS_CONNECTION_TIMEOUT_THRESHOLD;
    reset_health_check(client);
    record_successful_operation(client);
    
    // Initialize error tracking
    client->network_error_count = 0;
    client->protocol_error_count = 0;
    client->memory_error_count = 0;
    client->timeout_error_count = 0;
    client->tls_error_count = 0;
    client->unknown_error_count = 0;
    
    // Initialize recovery configuration with defaults
    client->max_network_errors = WS_NETWORK_ERROR_MAX_ATTEMPTS;
    client->max_protocol_errors = WS_PROTOCOL_ERROR_MAX_ATTEMPTS;
    client->max_memory_errors = WS_MEMORY_ERROR_MAX_ATTEMPTS;
    client->max_timeout_errors = WS_TIMEOUT_ERROR_MAX_ATTEMPTS;
    client->max_tls_errors = WS_TLS_ERROR_MAX_ATTEMPTS;
    client->max_unknown_errors = WS_UNKNOWN_ERROR_MAX_ATTEMPTS;
    client->memory_error_delay = WS_MEMORY_ERROR_DELAY_SECONDS;
    client->connection_stabilize_delay = WS_CONNECTION_STABILIZE_DELAY_SECONDS;
    
    // Initialize error info
    client->last_error.type = WS_ERROR_NONE;
    client->last_error.curl_code = CURLE_OK;
    client->last_error.message = NULL;
    client->last_error.timestamp = 0;
    
    return client;
}

void ws_client_free(ws_client_t *client) {
    if (!client) return;
    
    if (client->state == WS_STATE_CONNECTED) {
        ws_client_disconnect(client);
    }
    
    if (client->curl) {
        curl_easy_cleanup(client->curl);
    }
    
    free_tls_config(&client->tls);
    free(client->url);
    free(client->last_error.message);
    free(client);
}

bool ws_client_connect(ws_client_t *client) {
    if (!client || !client->curl) return false;
    
    if (client->state == WS_STATE_CONNECTED) return true;
    
    client->state = WS_STATE_CONNECTING;
    
    // Configure curl for WebSocket
    curl_easy_setopt(client->curl, CURLOPT_URL, client->url);
    curl_easy_setopt(client->curl, CURLOPT_CONNECT_ONLY, 2L); // WebSocket mode
    curl_easy_setopt(client->curl, CURLOPT_CONNECTTIMEOUT, client->connect_timeout);
    curl_easy_setopt(client->curl, CURLOPT_TIMEOUT, client->read_timeout);
    
    // Apply TLS configuration if enabled
    if (client->tls.enabled) {
        apply_tls_config(client->curl, &client->tls);
    }
    
    // Perform the connection
    CURLcode res = curl_easy_perform(client->curl);
    
    if (res != CURLE_OK) {
        client->state = WS_STATE_ERROR;
        ws_error_type_t error_type = curl_code_to_error_type(res);
        set_error(client, error_type, res, curl_easy_strerror(res));
        increment_error_counter(client, error_type);
        
        if (client->on_error) {
            client->on_error(&client->last_error, client->user_data);
        }
        
        // Use robust recovery strategy
        if (client->reconnect.enabled && should_attempt_reconnect(client)) {
            // Determine if we need full reconnection based on error type
            bool force_full_reconnect = (error_type == WS_ERROR_PROTOCOL || 
                                       error_type == WS_ERROR_MEMORY ||
                                       error_type == WS_ERROR_AUTH ||
                                       error_type == WS_ERROR_TLS);
            
            return ws_client_recover(client, force_full_reconnect);
        }
        
        return false;
    }
    
    client->state = WS_STATE_CONNECTED;
    client->reconnect.current_attempt = 0; // Reset on successful connection
    reset_health_check(client);
    record_successful_operation(client);
    
    // Wait for connection to stabilize
    usleep((unsigned int)(client->connection_stabilize_delay * 1000000));
    
    // Notify reconnection success if this was a reconnect attempt
    if (client->reconnect.current_attempt > 0 && client->on_reconnected) {
        client->on_reconnected(client->reconnect.current_attempt, client->user_data);
    }
    
    if (client->on_connect) {
        client->on_connect(client->user_data);
    }
    
    return true;
}

void ws_client_disconnect(ws_client_t *client) {
    if (!client || client->state != WS_STATE_CONNECTED) return;
    
    client->state = WS_STATE_CLOSING;
    
    // Send close frame
    size_t sent;
    curl_ws_send(client->curl, "", 0, &sent, 0, CURLWS_CLOSE);
    
    client->state = WS_STATE_DISCONNECTED;
    if (client->on_disconnect) {
        client->on_disconnect(client->user_data);
    }
}

bool ws_client_is_connected(ws_client_t *client) {
    return client && client->state == WS_STATE_CONNECTED;
}

bool ws_client_send_text(ws_client_t *client, const char *text) {
    if (!client || !text || client->state != WS_STATE_CONNECTED) return false;
    
    size_t sent;
    CURLcode res = curl_ws_send(client->curl, text, strlen(text), &sent, 0, CURLWS_TEXT);
    
    if (res != CURLE_OK) {
        ws_error_type_t error_type = curl_code_to_error_type(res);
        set_error(client, error_type, res, curl_easy_strerror(res));
        increment_error_counter(client, error_type);
        
        if (client->on_error) {
            client->on_error(&client->last_error, client->user_data);
        }
        
        // Use robust recovery strategy based on error type
        if (client->reconnect.enabled) {
            client->state = WS_STATE_ERROR;
            
            // Determine recovery strategy based on error type and count
            bool force_full_reconnect = (error_type == WS_ERROR_PROTOCOL || 
                                       error_type == WS_ERROR_MEMORY ||
                                       error_type == WS_ERROR_TLS ||
                                       ws_client_get_error_count(client, error_type) > 
                                       (error_type == WS_ERROR_NETWORK ? client->max_network_errors : 
                                        error_type == WS_ERROR_TIMEOUT ? client->max_timeout_errors : 
                                        error_type == WS_ERROR_TLS ? client->max_tls_errors :
                                        client->max_unknown_errors) / 2);
            
            if (should_attempt_reconnect(client)) {
                ws_client_recover(client, force_full_reconnect);
            }
        }
        
        return false;
    }
    
    record_successful_operation(client);
    return sent == strlen(text);
}

bool ws_client_send_binary(ws_client_t *client, const void *data, size_t length) {
    if (!client || !data || client->state != WS_STATE_CONNECTED) return false;
    
    size_t sent;
    CURLcode res = curl_ws_send(client->curl, data, length, &sent, 0, CURLWS_BINARY);
    
    if (res != CURLE_OK) {
        ws_error_type_t error_type = curl_code_to_error_type(res);
        set_error(client, error_type, res, curl_easy_strerror(res));
        increment_error_counter(client, error_type);
        
        if (client->on_error) {
            client->on_error(&client->last_error, client->user_data);
        }
        
        // Use robust recovery strategy
        if (client->reconnect.enabled) {
            client->state = WS_STATE_ERROR;
            
            bool force_full_reconnect = (error_type == WS_ERROR_PROTOCOL || 
                                       error_type == WS_ERROR_MEMORY ||
                                       error_type == WS_ERROR_TLS ||
                                       ws_client_get_error_count(client, error_type) > 
                                       client->max_network_errors / 2);
            
            if (should_attempt_reconnect(client)) {
                ws_client_recover(client, force_full_reconnect);
            }
        }
        
        return false;
    }
    
    record_successful_operation(client);
    return sent == length;
}

bool ws_client_send_ping(ws_client_t *client, const char *payload) {
    if (!client || client->state != WS_STATE_CONNECTED) return false;
    
    const char *data = payload ? payload : "";
    size_t length = payload ? strlen(payload) : 0;
    size_t sent;
    
    CURLcode res = curl_ws_send(client->curl, data, length, &sent, 0, CURLWS_PING);
    
    if (res != CURLE_OK) {
        ws_error_type_t error_type = curl_code_to_error_type(res);
        set_error(client, error_type, res, curl_easy_strerror(res));
        increment_error_counter(client, error_type);
        
        if (client->on_error) {
            client->on_error(&client->last_error, client->user_data);
        }
        
        // Use robust recovery strategy
        if (client->reconnect.enabled) {
            client->state = WS_STATE_ERROR;
            
            bool force_full_reconnect = (error_type == WS_ERROR_PROTOCOL || 
                                       error_type == WS_ERROR_MEMORY ||
                                       error_type == WS_ERROR_TLS ||
                                       ws_client_get_error_count(client, error_type) > 
                                       client->max_network_errors / 2);
            
            if (should_attempt_reconnect(client)) {
                ws_client_recover(client, force_full_reconnect);
            }
        }
        
        return false;
    }
    
    client->last_ping_sent = get_current_time();
    record_successful_operation(client);
    return true;
}

bool ws_client_send_pong(ws_client_t *client, const char *payload) {
    if (!client || client->state != WS_STATE_CONNECTED) return false;
    
    const char *data = payload ? payload : "";
    size_t length = payload ? strlen(payload) : 0;
    size_t sent;
    
    CURLcode res = curl_ws_send(client->curl, data, length, &sent, 0, CURLWS_PONG);
    
    if (res != CURLE_OK) {
        ws_error_type_t error_type = curl_code_to_error_type(res);
        set_error(client, error_type, res, curl_easy_strerror(res));
        increment_error_counter(client, error_type);
        
        if (client->on_error) {
            client->on_error(&client->last_error, client->user_data);
        }
        
        // Use robust recovery strategy
        if (client->reconnect.enabled) {
            client->state = WS_STATE_ERROR;
            
            bool force_full_reconnect = (error_type == WS_ERROR_PROTOCOL || 
                                       error_type == WS_ERROR_MEMORY ||
                                       error_type == WS_ERROR_TLS ||
                                       ws_client_get_error_count(client, error_type) > 
                                       client->max_network_errors / 2);
            
            if (should_attempt_reconnect(client)) {
                ws_client_recover(client, force_full_reconnect);
            }
        }
        
        return false;
    }
    
    record_successful_operation(client);
    return true;
}

ws_message_t* ws_client_recv(ws_client_t *client) {
    if (!client || client->state != WS_STATE_CONNECTED) return NULL;
    
    char buffer[4096];
    size_t received;
    const struct curl_ws_frame *frame;
    
    CURLcode res = curl_ws_recv(client->curl, buffer, sizeof(buffer), &received, &frame);
    
    if (res != CURLE_OK) {
        if (res != CURLE_AGAIN) { // CURLE_AGAIN means no data available, not an error
            ws_error_type_t error_type = curl_code_to_error_type(res);
            set_error(client, error_type, res, curl_easy_strerror(res));
            increment_error_counter(client, error_type);
            
            if (client->on_error) {
                client->on_error(&client->last_error, client->user_data);
            }
            
            // Use robust recovery strategy
            if (client->reconnect.enabled) {
                client->state = WS_STATE_ERROR;
                
                bool force_full_reconnect = (error_type == WS_ERROR_PROTOCOL || 
                                           error_type == WS_ERROR_MEMORY ||
                                           error_type == WS_ERROR_TLS ||
                                           ws_client_get_error_count(client, error_type) > 
                                           client->max_network_errors / 2);
                
                if (should_attempt_reconnect(client)) {
                    ws_client_recover(client, force_full_reconnect);
                }
            }
        }
        return NULL;
    }
    
    if (received == 0) return NULL;
    
    record_successful_operation(client);
    
    ws_message_t *message = calloc(1, sizeof(ws_message_t));
    if (!message) return NULL;
    
    message->type = frame_type_to_msg_type(frame->flags);
    message->length = received;
    message->is_fragment = !(frame->flags & CURLWS_CONT);
    message->is_final_fragment = (frame->flags & CURLWS_CONT) == 0;
    
    if (received > 0) {
        message->data = malloc(received + 1);
        if (message->data) {
            memcpy(message->data, buffer, received);
            message->data[received] = '\0'; // Null terminate for text messages
        } else {
            free(message);
            return NULL;
        }
    }
    
    // Update health check for pong messages
    update_health_check(client, message->type);
    
    return message;
}

void ws_message_free(ws_message_t *message) {
    if (!message) return;
    free(message->data);
    free(message);
}

void ws_client_set_timeout(ws_client_t *client, long connect_timeout, long read_timeout) {
    if (!client) return;
    client->connect_timeout = connect_timeout;
    client->read_timeout = read_timeout;
}

void ws_client_set_callbacks(ws_client_t *client,
                           void (*on_message)(ws_message_t *message, void *user_data),
                           void (*on_connect)(void *user_data),
                           void (*on_disconnect)(void *user_data),
                           void (*on_error)(ws_error_info_t *error, void *user_data),
                           void *user_data) {
    if (!client) return;
    
    client->on_message = on_message;
    client->on_connect = on_connect;
    client->on_disconnect = on_disconnect;
    client->on_error = on_error;
    client->user_data = user_data;
}

const char* ws_client_get_error_string(ws_client_t *client) {
    return client ? client->last_error.message : NULL;
}

ws_state_t ws_client_get_state(ws_client_t *client) {
    return client ? client->state : WS_STATE_ERROR;
}

// Extended callbacks for reconnection
void ws_client_set_reconnect_callbacks(ws_client_t *client,
                                     void (*on_reconnecting)(int attempt, double delay, void *user_data),
                                     void (*on_reconnected)(int attempts_used, void *user_data)) {
    if (!client) return;
    client->on_reconnecting = on_reconnecting;
    client->on_reconnected = on_reconnected;
}

// Reconnection configuration
void ws_client_enable_reconnect(ws_client_t *client, int max_attempts, 
                              double initial_interval, double max_interval) {
    if (!client) return;
    client->reconnect.enabled = true;
    client->reconnect.max_attempts = max_attempts;
    client->reconnect.initial_interval = initial_interval;
    client->reconnect.max_interval = max_interval;
    client->reconnect.current_attempt = 0;
}

void ws_client_disable_reconnect(ws_client_t *client) {
    if (!client) return;
    client->reconnect.enabled = false;
}

void ws_client_set_reconnect_backoff(ws_client_t *client, double multiplier, double jitter_factor) {
    if (!client) return;
    client->reconnect.backoff_multiplier = multiplier;
    client->reconnect.jitter_factor = jitter_factor;
}

// Health monitoring
void ws_client_set_ping_config(ws_client_t *client, double ping_interval, double pong_timeout) {
    if (!client) return;
    client->ping_interval = ping_interval;
    client->pong_timeout = pong_timeout;
}

bool ws_client_is_healthy(ws_client_t *client) {
    if (!client || client->state != WS_STATE_CONNECTED) return false;
    
    double current_time = get_current_time();
    
    // Check if we're waiting for a pong response
    if (client->last_ping_sent > 0) {
        double pong_wait_time = current_time - client->last_ping_sent;
        if (pong_wait_time > client->pong_timeout) {
            return false; // Pong timeout
        }
    }
    
    return true;
}

void ws_client_send_keepalive(ws_client_t *client) {
    if (!client || client->state != WS_STATE_CONNECTED) return;
    
    double current_time = get_current_time();
    
    // Send ping if interval has passed
    if (current_time - client->last_ping_sent >= client->ping_interval) {
        if (ws_client_send_ping(client, "keepalive")) {
            client->last_ping_sent = current_time;
        }
    }
}

// Manual reconnection control
bool ws_client_reconnect(ws_client_t *client) {
    if (!client) return false;
    
    if (client->state == WS_STATE_CONNECTED) {
        ws_client_disconnect(client);
    }
    
    client->reconnect.current_attempt++;
    return ws_client_connect(client);
}

void ws_client_reset_reconnect_attempts(ws_client_t *client) {
    if (!client) return;
    client->reconnect.current_attempt = 0;
}

// Updated utility functions
ws_error_info_t* ws_client_get_error(ws_client_t *client) {
    return client ? &client->last_error : NULL;
}

int ws_client_get_reconnect_attempts(ws_client_t *client) {
    return client ? client->reconnect.current_attempt : 0;
}

// Enhanced error handling configuration
void ws_client_set_error_limits(ws_client_t *client, 
                              int max_network_errors, int max_protocol_errors,
                              int max_memory_errors, int max_timeout_errors,
                              int max_tls_errors, int max_unknown_errors) {
    if (!client) return;
    client->max_network_errors = max_network_errors;
    client->max_protocol_errors = max_protocol_errors;
    client->max_memory_errors = max_memory_errors;
    client->max_timeout_errors = max_timeout_errors;
    client->max_tls_errors = max_tls_errors;
    client->max_unknown_errors = max_unknown_errors;
}

void ws_client_set_recovery_config(ws_client_t *client,
                                 double memory_error_delay,
                                 double connection_stabilize_delay,
                                 double connection_timeout_threshold) {
    if (!client) return;
    client->memory_error_delay = memory_error_delay;
    client->connection_stabilize_delay = connection_stabilize_delay;
    client->connection_timeout_threshold = connection_timeout_threshold;
}

// Enhanced callbacks for recovery failure
void ws_client_set_recovery_callback(ws_client_t *client,
                                   void (*on_recovery_failed)(void *user_data)) {
    if (!client) return;
    client->on_recovery_failed = on_recovery_failed;
}

// Enhanced health monitoring
bool ws_client_check_connection_health(ws_client_t *client) {
    if (!client || client->state != WS_STATE_CONNECTED) return false;
    
    time_t current_time = time(NULL);
    
    // Check if too much time has passed without successful operations
    if (current_time - client->last_successful_operation > client->connection_timeout_threshold) {
        if (client->reconnect.enabled) {
            client->state = WS_STATE_ERROR;
            ws_client_recover(client, true); // Force full reconnection for health timeout
        }
        return false;
    }
    
    return ws_client_is_healthy(client);
}

// Enhanced reconnection control with recovery strategies
bool ws_client_lightweight_reconnect(ws_client_t *client) {
    if (!client) return false;
    return ws_client_recover(client, false);
}

bool ws_client_full_reconnect(ws_client_t *client) {
    if (!client) return false;
    return ws_client_recover(client, true);
}

bool ws_client_recover_from_error(ws_client_t *client, ws_error_type_t error_type) {
    if (!client) return false;
    
    increment_error_counter(client, error_type);
    
    // Check if we've exceeded error limits for this type
    int error_count = ws_client_get_error_count(client, error_type);
    bool force_full_reconnect = false;
    bool should_delay = false;
    
    switch (error_type) {
        case WS_ERROR_NETWORK:
            force_full_reconnect = (error_count > client->max_network_errors / 2);
            break;
        case WS_ERROR_PROTOCOL:
            force_full_reconnect = true; // Always force full reconnect for protocol errors
            break;
        case WS_ERROR_MEMORY:
            force_full_reconnect = true;
            should_delay = true; // Wait before retrying memory errors
            break;
        case WS_ERROR_TIMEOUT:
            force_full_reconnect = (error_count > client->max_timeout_errors / 2);
            break;
        case WS_ERROR_AUTH:
            force_full_reconnect = true;
            break;
        case WS_ERROR_TLS:
            force_full_reconnect = true; // Always force full reconnect for TLS errors
            break;
        default:
            force_full_reconnect = (error_count > client->max_unknown_errors / 2);
            break;
    }
    
    // Apply delay for memory errors
    if (should_delay) {
        usleep((unsigned int)(client->memory_error_delay * 1000000));
    }
    
    return ws_client_recover(client, force_full_reconnect);
}

void ws_client_reset_error_counters(ws_client_t *client) {
    if (!client) return;
    
    client->network_error_count = 0;
    client->protocol_error_count = 0;
    client->memory_error_count = 0;
    client->timeout_error_count = 0;
    client->tls_error_count = 0;
    client->unknown_error_count = 0;
}

int ws_client_get_error_count(ws_client_t *client, ws_error_type_t error_type) {
    if (!client) return 0;
    
    switch (error_type) {
        case WS_ERROR_NETWORK:
            return client->network_error_count;
        case WS_ERROR_PROTOCOL:
            return client->protocol_error_count;
        case WS_ERROR_MEMORY:
            return client->memory_error_count;
        case WS_ERROR_TIMEOUT:
            return client->timeout_error_count;
        case WS_ERROR_TLS:
            return client->tls_error_count;
        case WS_ERROR_UNKNOWN:
            return client->unknown_error_count;
        default:
            return 0;
    }
}

time_t ws_client_get_last_successful_operation(ws_client_t *client) {
    return client ? client->last_successful_operation : 0;
}

// Helper functions
static void set_error(ws_client_t *client, ws_error_type_t type, int curl_code, const char *message) {
    if (!client || !message) return;
    
    free(client->last_error.message);
    client->last_error.type = type;
    client->last_error.curl_code = curl_code;
    client->last_error.message = strdup(message);
    client->last_error.timestamp = time(NULL);
}

static ws_msg_type_t frame_type_to_msg_type(int flags) {
    if (flags & CURLWS_TEXT) return WS_MSG_TEXT;
    if (flags & CURLWS_BINARY) return WS_MSG_BINARY;
    if (flags & CURLWS_PING) return WS_MSG_PING;
    if (flags & CURLWS_PONG) return WS_MSG_PONG;
    if (flags & CURLWS_CLOSE) return WS_MSG_CLOSE;
    return WS_MSG_TEXT; // Default fallback
}

static int msg_type_to_curl_flags(ws_msg_type_t type) {
    switch (type) {
        case WS_MSG_TEXT: return CURLWS_TEXT;
        case WS_MSG_BINARY: return CURLWS_BINARY;
        case WS_MSG_PING: return CURLWS_PING;
        case WS_MSG_PONG: return CURLWS_PONG;
        case WS_MSG_CLOSE: return CURLWS_CLOSE;
        default: return CURLWS_TEXT;
    }
}

static ws_error_type_t curl_code_to_error_type(CURLcode code) {
    switch (code) {
        case CURLE_OPERATION_TIMEDOUT:
            return WS_ERROR_TIMEOUT;
        case CURLE_COULDNT_CONNECT:
        case CURLE_COULDNT_RESOLVE_HOST:
        case CURLE_COULDNT_RESOLVE_PROXY:
        case CURLE_SEND_ERROR:
        case CURLE_RECV_ERROR:
            return WS_ERROR_NETWORK;
        case CURLE_OUT_OF_MEMORY:
            return WS_ERROR_MEMORY;
        case CURLE_LOGIN_DENIED:
        case CURLE_AUTH_ERROR:
            return WS_ERROR_AUTH;
        case CURLE_SSL_CONNECT_ERROR:
        case CURLE_SSL_ENGINE_NOTFOUND:
        case CURLE_SSL_ENGINE_SETFAILED:
        case CURLE_SSL_CERTPROBLEM:
        case CURLE_SSL_CIPHER:
        case CURLE_SSL_CACERT:
        case CURLE_SSL_CACERT_BADFILE:
        case CURLE_SSL_CRL_BADFILE:
        case CURLE_SSL_ISSUER_ERROR:
        case CURLE_SSL_PINNEDPUBKEYNOTMATCH:
        case CURLE_SSL_INVALIDCERTSTATUS:
        case CURLE_SSL_CLIENTCERT:
            return WS_ERROR_TLS;
        case CURLE_UNSUPPORTED_PROTOCOL:
        case CURLE_BAD_FUNCTION_ARGUMENT:
            return WS_ERROR_PROTOCOL;
        default:
            return WS_ERROR_UNKNOWN;
    }
}

static double get_current_time(void) {
    return (double)time(NULL);
}

static double calculate_reconnect_delay(ws_client_t *client) {
    if (!client) return 0.0;
    
    double delay = client->reconnect.initial_interval * 
                   pow(client->reconnect.backoff_multiplier, client->reconnect.current_attempt - 1);
    
    // Apply maximum interval limit
    if (delay > client->reconnect.max_interval) {
        delay = client->reconnect.max_interval;
    }
    
    // Add jitter to avoid thundering herd
    if (client->reconnect.jitter_factor > 0) {
        double jitter = delay * client->reconnect.jitter_factor * ((double)rand() / RAND_MAX);
        delay += jitter;
    }
    
    return delay;
}

static bool should_attempt_reconnect(ws_client_t *client) {
    if (!client || !client->reconnect.enabled) return false;
    
    if (client->reconnect.max_attempts > 0 && 
        client->reconnect.current_attempt >= client->reconnect.max_attempts) {
        return false;
    }
    
    return true;
}

static void reset_health_check(ws_client_t *client) {
    if (!client) return;
    
    double current_time = get_current_time();
    client->last_ping_sent = 0;
    client->last_pong_received = current_time;
}

static void update_health_check(ws_client_t *client, ws_msg_type_t msg_type) {
    if (!client) return;
    
    double current_time = get_current_time();
    
    if (msg_type == WS_MSG_PONG) {
        client->last_pong_received = current_time;
        client->last_ping_sent = 0; // Reset ping timer
    }
}

static void increment_error_counter(ws_client_t *client, ws_error_type_t error_type) {
    if (!client) return;
    
    switch (error_type) {
        case WS_ERROR_NETWORK:
            client->network_error_count++;
            break;
        case WS_ERROR_PROTOCOL:
            client->protocol_error_count++;
            break;
        case WS_ERROR_MEMORY:
            client->memory_error_count++;
            break;
        case WS_ERROR_TIMEOUT:
            client->timeout_error_count++;
            break;
        case WS_ERROR_TLS:
            client->tls_error_count++;
            break;
        case WS_ERROR_UNKNOWN:
            client->unknown_error_count++;
            break;
        default:
            break;
    }
}

static bool ws_client_recover(ws_client_t *client, bool force_full_reconnect) {
    if (!client) return false;
    
    static int recovery_attempt = 0;
    
    while (recovery_attempt < client->reconnect.max_attempts) {
        recovery_attempt++;
        
        // Exponential backoff with jitter
        double delay = calculate_reconnect_delay(client);
        
        if (client->on_reconnecting) {
            client->on_reconnecting(recovery_attempt, delay, client->user_data);
        }
        
        // Sleep for the calculated delay
        usleep((unsigned int)(delay * 1000000));
        
        // Strategy 1: Try lightweight reconnect first (unless forced to skip)
        if (!force_full_reconnect && client->state == WS_STATE_CONNECTED) {
            // Already connected, just need to reset error counters
            ws_client_reset_error_counters(client);
            record_successful_operation(client);
            recovery_attempt = 0;
            return true;
        }
        
        if (!force_full_reconnect) {
            // Try to reconnect using existing curl handle
            if (client->state != WS_STATE_CONNECTED) {
                client->state = WS_STATE_CONNECTING;
                
                CURLcode res = curl_easy_perform(client->curl);
                if (res == CURLE_OK) {
                    client->state = WS_STATE_CONNECTED;
                    client->reconnect.current_attempt = 0;
                    ws_client_reset_error_counters(client);
                    reset_health_check(client);
                    record_successful_operation(client);
                    
                    // Wait for connection to stabilize
                    usleep((unsigned int)(client->connection_stabilize_delay * 1000000));
                    
                    if (client->on_reconnected) {
                        client->on_reconnected(recovery_attempt, client->user_data);
                    }
                    
                    recovery_attempt = 0;
                    return true;
                }
            }
        }
        
        // Strategy 2: Try complete reinitialization as fallback
        if (client->curl) {
            curl_easy_cleanup(client->curl);
        }
        
        client->curl = curl_easy_init();
        if (!client->curl) {
            continue; // Try next attempt
        }
        
        // Reconfigure curl for WebSocket
        curl_easy_setopt(client->curl, CURLOPT_URL, client->url);
        curl_easy_setopt(client->curl, CURLOPT_CONNECT_ONLY, 2L); // WebSocket mode
        curl_easy_setopt(client->curl, CURLOPT_CONNECTTIMEOUT, client->connect_timeout);
        curl_easy_setopt(client->curl, CURLOPT_TIMEOUT, client->read_timeout);
        
        // Attempt new connection
        CURLcode res = curl_easy_perform(client->curl);
        if (res == CURLE_OK) {
            client->state = WS_STATE_CONNECTED;
            client->reconnect.current_attempt = 0;
            ws_client_reset_error_counters(client);
            reset_health_check(client);
            record_successful_operation(client);
            
            // Wait for connection to stabilize
            usleep((unsigned int)(client->connection_stabilize_delay * 1000000));
            
            if (client->on_reconnected) {
                client->on_reconnected(recovery_attempt, client->user_data);
            }
            
            recovery_attempt = 0;
            return true;
        } else {
            ws_error_type_t error_type = curl_code_to_error_type(res);
            set_error(client, error_type, res, curl_easy_strerror(res));
        }
    }
    
    // If we get here, all recovery attempts failed
    client->state = WS_STATE_ERROR;
    if (client->on_recovery_failed) {
        client->on_recovery_failed(client->user_data);
    }
    
    recovery_attempt = 0;
    return false;
}

static void record_successful_operation(ws_client_t *client) {
    if (!client) return;
    
    client->last_successful_operation = time(NULL);
    
    // Reset all error counters on successful operation
    client->network_error_count = 0;
    client->protocol_error_count = 0;
    client->memory_error_count = 0;
    client->timeout_error_count = 0;
    client->tls_error_count = 0;
    client->unknown_error_count = 0;
}

// TLS/SSL configuration functions
void ws_client_enable_tls(ws_client_t *client) {
    if (!client) return;
    client->tls.enabled = true;
}

void ws_client_disable_tls(ws_client_t *client) {
    if (!client) return;
    client->tls.enabled = false;
}

void ws_client_set_tls_cert(ws_client_t *client, const char *cert_file, const char *key_file, const char *key_password) {
    if (!client) return;
    
    free(client->tls.cert_file);
    free(client->tls.key_file);
    free(client->tls.key_password);
    
    client->tls.cert_file = cert_file ? strdup(cert_file) : NULL;
    client->tls.key_file = key_file ? strdup(key_file) : NULL;
    client->tls.key_password = key_password ? strdup(key_password) : NULL;
}

void ws_client_set_tls_ca_bundle(ws_client_t *client, const char *ca_bundle_file) {
    if (!client) return;
    
    free(client->tls.ca_bundle_file);
    client->tls.ca_bundle_file = ca_bundle_file ? strdup(ca_bundle_file) : NULL;
}

void ws_client_set_tls_ca_dir(ws_client_t *client, const char *ca_cert_dir) {
    if (!client) return;
    
    free(client->tls.ca_cert_dir);
    client->tls.ca_cert_dir = ca_cert_dir ? strdup(ca_cert_dir) : NULL;
}

void ws_client_set_tls_crl(ws_client_t *client, const char *crl_file) {
    if (!client) return;
    
    free(client->tls.crl_file);
    client->tls.crl_file = crl_file ? strdup(crl_file) : NULL;
}

void ws_client_set_tls_ciphers(ws_client_t *client, const char *cipher_list, const char *tls13_ciphers) {
    if (!client) return;
    
    free(client->tls.cipher_list);
    free(client->tls.tls13_ciphers);
    
    client->tls.cipher_list = cipher_list ? strdup(cipher_list) : NULL;
    client->tls.tls13_ciphers = tls13_ciphers ? strdup(tls13_ciphers) : NULL;
}

void ws_client_set_tls_verification(ws_client_t *client, bool verify_peer, bool verify_host, bool verify_status) {
    if (!client) return;
    
    client->tls.verify_peer = verify_peer;
    client->tls.verify_host = verify_host;
    client->tls.verify_status = verify_status;
}

void ws_client_set_tls_version(ws_client_t *client, long ssl_version) {
    if (!client) return;
    client->tls.ssl_version = ssl_version;
}

void ws_client_set_tls_options(ws_client_t *client, long ssl_options, bool allow_beast, bool no_revoke) {
    if (!client) return;
    
    client->tls.ssl_options = ssl_options;
    client->tls.allow_beast = allow_beast;
    client->tls.no_revoke = no_revoke;
}

bool ws_client_is_tls_enabled(ws_client_t *client) {
    return client ? client->tls.enabled : false;
}

static void init_tls_config(ws_tls_config_t *tls) {
    if (!tls) return;
    
    memset(tls, 0, sizeof(ws_tls_config_t));
    
    // Set secure defaults
    tls->enabled = false;
    tls->verify_peer = true;
    tls->verify_host = true;
    tls->verify_status = false;
    tls->ssl_version = CURL_SSLVERSION_DEFAULT;
    tls->ssl_options = 0;
    tls->allow_beast = false;
    tls->no_revoke = false;
}

static void free_tls_config(ws_tls_config_t *tls) {
    if (!tls) return;
    
    free(tls->cert_file);
    free(tls->key_file);
    free(tls->key_password);
    free(tls->ca_bundle_file);
    free(tls->ca_cert_dir);
    free(tls->crl_file);
    free(tls->cipher_list);
    free(tls->tls13_ciphers);
    
    // Clear all pointers to avoid double-free
    memset(tls, 0, sizeof(ws_tls_config_t));
}

static void apply_tls_config(CURL *curl, const ws_tls_config_t *tls) {
    if (!curl || !tls || !tls->enabled) return;
    
    // Set SSL/TLS version
    if (tls->ssl_version != 0) {
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, tls->ssl_version);
    }
    
    // Set SSL options
    if (tls->ssl_options != 0) {
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, tls->ssl_options);
    }
    
    // Set client certificate and key
    if (tls->cert_file) {
        curl_easy_setopt(curl, CURLOPT_SSLCERT, tls->cert_file);
    }
    if (tls->key_file) {
        curl_easy_setopt(curl, CURLOPT_SSLKEY, tls->key_file);
    }
    if (tls->key_password) {
        curl_easy_setopt(curl, CURLOPT_KEYPASSWD, tls->key_password);
    }
    
    // Set CA bundle or directory
    if (tls->ca_bundle_file) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, tls->ca_bundle_file);
    }
    if (tls->ca_cert_dir) {
        curl_easy_setopt(curl, CURLOPT_CAPATH, tls->ca_cert_dir);
    }
    
    // Set Certificate Revocation List
    if (tls->crl_file) {
        curl_easy_setopt(curl, CURLOPT_CRLFILE, tls->crl_file);
    }
    
    // Set cipher lists
    if (tls->cipher_list) {
        curl_easy_setopt(curl, CURLOPT_SSL_CIPHER_LIST, tls->cipher_list);
    }
    if (tls->tls13_ciphers) {
        curl_easy_setopt(curl, CURLOPT_TLS13_CIPHERS, tls->tls13_ciphers);
    }
    
    // Set verification options
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, tls->verify_peer ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, tls->verify_host ? 2L : 0L);
    
    if (tls->verify_status) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 1L);
    }
    
    // Set additional security options
    if (tls->allow_beast) {
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_ALLOW_BEAST);
    }
    
    if (tls->no_revoke) {
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NO_REVOKE);
    }
}

