#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <curl/curl.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// WebSocket connection states
typedef enum {
    WS_STATE_DISCONNECTED,
    WS_STATE_CONNECTING,
    WS_STATE_CONNECTED,
    WS_STATE_CLOSING,
    WS_STATE_RECONNECTING,
    WS_STATE_ERROR
} ws_state_t;

// Error types for better error handling
typedef enum {
    WS_ERROR_NONE,
    WS_ERROR_NETWORK,
    WS_ERROR_PROTOCOL,
    WS_ERROR_TIMEOUT,
    WS_ERROR_AUTH,
    WS_ERROR_MEMORY,
    WS_ERROR_TLS,
    WS_ERROR_UNKNOWN
} ws_error_type_t;

// WebSocket message types
typedef enum {
    WS_MSG_TEXT,
    WS_MSG_BINARY,
    WS_MSG_PING,
    WS_MSG_PONG,
    WS_MSG_CLOSE
} ws_msg_type_t;

// WebSocket message structure
typedef struct {
    ws_msg_type_t type;
    char *data;
    size_t length;
    bool is_fragment;
    bool is_final_fragment;
} ws_message_t;

// Reconnection configuration
typedef struct {
    bool enabled;
    int max_attempts;
    int current_attempt;
    double initial_interval;
    double max_interval;
    double backoff_multiplier;
    double jitter_factor;
    time_t last_attempt_time;
} ws_reconnect_config_t;

// Error information structure
typedef struct {
    ws_error_type_t type;
    int curl_code;
    char *message;
    time_t timestamp;
} ws_error_info_t;

// TLS/SSL configuration
typedef struct {
    bool enabled;
    char *cert_file;           // Client certificate file path
    char *key_file;            // Private key file path
    char *key_password;        // Password for private key (if encrypted)
    char *ca_bundle_file;      // CA bundle file path
    char *ca_cert_dir;         // CA certificate directory path
    char *crl_file;            // Certificate Revocation List file
    char *cipher_list;         // List of allowed ciphers
    char *tls13_ciphers;       // TLS 1.3 cipher suites
    bool verify_peer;          // Verify the peer's SSL certificate
    bool verify_host;          // Verify the certificate's name against host
    bool verify_status;        // Verify certificate status via OCSP stapling
    long ssl_version;          // SSL/TLS version to use (CURL_SSLVERSION_*)
    long ssl_options;          // SSL options (CURLSSLOPT_*)
    bool allow_beast;          // Allow BEAST SSL vulnerability
    bool no_revoke;            // Don't check for certificate revocation
} ws_tls_config_t;

// WebSocket client structure
typedef struct {
    CURL *curl;
    char *url;
    ws_state_t state;
    ws_error_info_t last_error;
    
    // Connection options
    long connect_timeout;
    long read_timeout;
    
    // TLS/SSL configuration
    ws_tls_config_t tls;
    
    // Reconnection configuration
    ws_reconnect_config_t reconnect;
    
    // Health check and connection monitoring
    time_t last_ping_sent;
    time_t last_pong_received;
    time_t last_successful_operation;
    double ping_interval;
    double pong_timeout;
    double connection_timeout_threshold;  // Time without successful ops before forcing reconnect
    
    // Error tracking and recovery strategy
    int network_error_count;
    int protocol_error_count;
    int memory_error_count;
    int timeout_error_count;
    int tls_error_count;
    int unknown_error_count;
    
    // Recovery configuration
    int max_network_errors;
    int max_protocol_errors;
    int max_memory_errors;
    int max_timeout_errors;
    int max_tls_errors;
    int max_unknown_errors;
    double memory_error_delay;
    double connection_stabilize_delay;
    
    // Callbacks
    void (*on_message)(ws_message_t *message, void *user_data);
    void (*on_connect)(void *user_data);
    void (*on_disconnect)(void *user_data);
    void (*on_error)(ws_error_info_t *error, void *user_data);
    void (*on_reconnecting)(int attempt, double delay, void *user_data);
    void (*on_reconnected)(int attempts_used, void *user_data);
    void (*on_recovery_failed)(void *user_data);  // Called when all recovery attempts fail
    void *user_data;
} ws_client_t;

// Function declarations
ws_client_t* ws_client_new(const char *url);
void ws_client_free(ws_client_t *client);

// Connection management
bool ws_client_connect(ws_client_t *client);
void ws_client_disconnect(ws_client_t *client);
bool ws_client_is_connected(ws_client_t *client);

// Message sending
bool ws_client_send_text(ws_client_t *client, const char *text);
bool ws_client_send_binary(ws_client_t *client, const void *data, size_t length);
bool ws_client_send_ping(ws_client_t *client, const char *payload);
bool ws_client_send_pong(ws_client_t *client, const char *payload);

// Message receiving
ws_message_t* ws_client_recv(ws_client_t *client);
void ws_message_free(ws_message_t *message);

// Configuration
void ws_client_set_timeout(ws_client_t *client, long connect_timeout, long read_timeout);
void ws_client_set_callbacks(ws_client_t *client,
                           void (*on_message)(ws_message_t *message, void *user_data),
                           void (*on_connect)(void *user_data),
                           void (*on_disconnect)(void *user_data),
                           void (*on_error)(ws_error_info_t *error, void *user_data),
                           void *user_data);

// TLS/SSL configuration
void ws_client_enable_tls(ws_client_t *client);
void ws_client_disable_tls(ws_client_t *client);
void ws_client_set_tls_cert(ws_client_t *client, const char *cert_file, const char *key_file, const char *key_password);
void ws_client_set_tls_ca_bundle(ws_client_t *client, const char *ca_bundle_file);
void ws_client_set_tls_ca_dir(ws_client_t *client, const char *ca_cert_dir);
void ws_client_set_tls_crl(ws_client_t *client, const char *crl_file);
void ws_client_set_tls_ciphers(ws_client_t *client, const char *cipher_list, const char *tls13_ciphers);
void ws_client_set_tls_verification(ws_client_t *client, bool verify_peer, bool verify_host, bool verify_status);
void ws_client_set_tls_version(ws_client_t *client, long ssl_version);
void ws_client_set_tls_options(ws_client_t *client, long ssl_options, bool allow_beast, bool no_revoke);
bool ws_client_is_tls_enabled(ws_client_t *client);

// Extended callbacks for reconnection
void ws_client_set_reconnect_callbacks(ws_client_t *client,
                                     void (*on_reconnecting)(int attempt, double delay, void *user_data),
                                     void (*on_reconnected)(int attempts_used, void *user_data));

// Extended callbacks for recovery failure
void ws_client_set_recovery_callback(ws_client_t *client,
                                   void (*on_recovery_failed)(void *user_data));

// Reconnection configuration
void ws_client_enable_reconnect(ws_client_t *client, int max_attempts, 
                              double initial_interval, double max_interval);
void ws_client_disable_reconnect(ws_client_t *client);
void ws_client_set_reconnect_backoff(ws_client_t *client, double multiplier, double jitter_factor);

// Enhanced error handling configuration
void ws_client_set_error_limits(ws_client_t *client, 
                              int max_network_errors, int max_protocol_errors,
                              int max_memory_errors, int max_timeout_errors,
                              int max_tls_errors, int max_unknown_errors);
void ws_client_set_recovery_config(ws_client_t *client,
                                 double memory_error_delay,
                                 double connection_stabilize_delay,
                                 double connection_timeout_threshold);

// Health monitoring
void ws_client_set_ping_config(ws_client_t *client, double ping_interval, double pong_timeout);
bool ws_client_is_healthy(ws_client_t *client);
void ws_client_send_keepalive(ws_client_t *client);
bool ws_client_check_connection_health(ws_client_t *client);

// Enhanced reconnection control with recovery strategies
bool ws_client_lightweight_reconnect(ws_client_t *client);
bool ws_client_full_reconnect(ws_client_t *client);
bool ws_client_recover_from_error(ws_client_t *client, ws_error_type_t error_type);

// Manual reconnection control
bool ws_client_reconnect(ws_client_t *client);
void ws_client_reset_reconnect_attempts(ws_client_t *client);
void ws_client_reset_error_counters(ws_client_t *client);

// Utility functions
ws_error_info_t* ws_client_get_error(ws_client_t *client);
const char* ws_client_get_error_string(ws_client_t *client);
ws_state_t ws_client_get_state(ws_client_t *client);
int ws_client_get_reconnect_attempts(ws_client_t *client);
int ws_client_get_error_count(ws_client_t *client, ws_error_type_t error_type);
time_t ws_client_get_last_successful_operation(ws_client_t *client);

#endif // WEBSOCKET_H
