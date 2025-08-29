#ifndef UBUS_CLIENT_H
#define UBUS_CLIENT_H

#include <json-c/json.h>
#include <libubox/blobmsg.h>
#include <libubus.h>
#include <stdbool.h>

// Default timeout for UBUS calls (milliseconds)
#define UBUS_CLIENT_DEFAULT_TIMEOUT 5000

// Maximum response size for UBUS calls
#define UBUS_CLIENT_MAX_RESPONSE_SIZE 4096

// UBUS client context structure
typedef struct {
    struct ubus_context *ctx;
    bool connected;
    int timeout_ms;
} UbusClient;

// UBUS response structure
typedef struct {
    bool success;
    int error_code;
    char *error_message;
    struct blob_attr *data;
    char *json_response;
} UbusResponse;

// Callback function type for asynchronous UBUS calls
typedef void (*UbusCallback)(UbusResponse *response, void *user_data);

/**
 * Initialize UBUS client
 * @param timeout_ms Timeout for UBUS calls in milliseconds (0 for default)
 * @return Pointer to UbusClient on success, NULL on failure
 */
UbusClient *ubus_client_init(int timeout_ms);

/**
 * Cleanup UBUS client
 * @param client Pointer to UbusClient to cleanup
 */
void ubus_client_cleanup(UbusClient *client);

/**
 * Check if client is connected
 * @param client Pointer to UbusClient
 * @return true if connected, false otherwise
 */
bool ubus_client_is_connected(UbusClient *client);

/**
 * Call a UBUS method synchronously
 * @param client Pointer to UbusClient
 * @param service_name Name of the UBUS service to call
 * @param method_name Name of the method to call
 * @param args Arguments for the method (can be NULL for no args)
 * @return UbusResponse structure (caller must free with ubus_response_free)
 */
UbusResponse *
ubus_client_call(UbusClient *client, const char *service_name, const char *method_name, struct blob_attr *args);

/**
 * Call a UBUS method with JSON arguments
 * @param client Pointer to UbusClient
 * @param service_name Name of the UBUS service to call
 * @param method_name Name of the method to call
 * @param json_args JSON string with arguments (can be NULL)
 * @return UbusResponse structure (caller must free with ubus_response_free)
 */
UbusResponse *
ubus_client_call_json(UbusClient *client, const char *service_name, const char *method_name, const char *json_args);

/**
 * Call a UBUS method asynchronously
 * @param client Pointer to UbusClient
 * @param service_name Name of the UBUS service to call
 * @param method_name Name of the method to call
 * @param args Arguments for the method (can be NULL for no args)
 * @param callback Callback function to call when response is received
 * @param user_data User data to pass to callback
 * @return 0 on success, negative error code on failure
 */
int ubus_client_call_async(UbusClient *client,
                           const char *service_name,
                           const char *method_name,
                           struct blob_attr *args,
                           UbusCallback callback,
                           void *user_data);

/**
 * List available UBUS services
 * @param client Pointer to UbusClient
 * @return UbusResponse with list of services (caller must free)
 */
UbusResponse *ubus_client_list_services(UbusClient *client);

/**
 * Get methods for a specific service
 * @param client Pointer to UbusClient
 * @param service_name Name of the service
 * @return UbusResponse with service methods (caller must free)
 */
UbusResponse *ubus_client_get_service_methods(UbusClient *client, const char *service_name);

/**
 * Send a simple ping to a service
 * @param client Pointer to UbusClient
 * @param service_name Name of the service to ping
 * @return true if service responded, false otherwise
 */
bool ubus_client_ping_service(UbusClient *client, const char *service_name);

/**
 * Free UBUS response structure
 * @param response Pointer to UbusResponse to free
 */
void ubus_response_free(UbusResponse *response);

/**
 * Get string value from UBUS response
 * @param response Pointer to UbusResponse
 * @param key Key to look for in response
 * @return String value or NULL if not found
 */
const char *ubus_response_get_string(UbusResponse *response, const char *key);

/**
 * Get integer value from UBUS response
 * @param response Pointer to UbusResponse
 * @param key Key to look for in response
 * @param default_value Default value if key not found
 * @return Integer value or default_value if not found
 */
int ubus_response_get_int(UbusResponse *response, const char *key, int default_value);

/**
 * Get boolean value from UBUS response
 * @param response Pointer to UbusResponse
 * @param key Key to look for in response
 * @param default_value Default value if key not found
 * @return Boolean value or default_value if not found
 */
bool ubus_response_get_bool(UbusResponse *response, const char *key, bool default_value);

/**
 * Convert UBUS response to JSON string
 * @param response Pointer to UbusResponse
 * @return JSON string representation (caller must free) or NULL on error
 */
char *ubus_response_to_json(UbusResponse *response);

/**
 * Helper function to create blob_attr from JSON string
 * @param json_str JSON string to convert
 * @return blob_attr structure (caller must free) or NULL on error
 */
struct blob_attr *ubus_client_json_to_blob(const char *json_str);

/**
 * Helper function to create JSON string from blob_attr
 * @param attr blob_attr to convert
 * @return JSON string (caller must free) or NULL on error
 */
char *ubus_client_blob_to_json(struct blob_attr *attr);

#endif // UBUS_CLIENT_H