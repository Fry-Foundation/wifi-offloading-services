#include "ubus_client.h"
#include "core/console.h"
#include <json-c/json.h>
#include <libubox/blobmsg.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static Console csl = {
    .topic = "ubus_client",
};

// Structure for async call context
typedef struct {
    UbusCallback callback;
    void *user_data;
    UbusClient *client;
    bool completed;
} AsyncCallContext;

// Helper function to create UbusResponse
static UbusResponse *create_response(bool success, int error_code, const char *error_message) {
    UbusResponse *response = malloc(sizeof(UbusResponse));
    if (!response) {
        return NULL;
    }

    memset(response, 0, sizeof(UbusResponse));
    response->success = success;
    response->error_code = error_code;

    if (error_message) {
        response->error_message = strdup(error_message);
    }

    return response;
}

// Callback for synchronous UBUS calls
static void sync_call_callback(struct ubus_request *req, int type, struct blob_attr *msg) {
    UbusResponse **response_ptr = (UbusResponse **)req->priv;

    if (!response_ptr || !*response_ptr) {
        return;
    }

    UbusResponse *response = *response_ptr;

    if (msg) {
        // Clone the blob data
        int len = blob_len(msg) + sizeof(struct blob_attr);
        response->data = malloc(len);
        if (response->data) {
            memcpy(response->data, msg, len);

            // Convert to JSON for easier access
            response->json_response = blobmsg_format_json(msg, true);
        }
    }

    response->success = true;
}

// Note: Async callback removed as we're using simplified synchronous approach for now

// Initialize UBUS client
UbusClient *ubus_client_init(int timeout_ms) {
    console_debug(&csl, "Initializing UBUS client");

    UbusClient *client = malloc(sizeof(UbusClient));
    if (!client) {
        console_error(&csl, "Failed to allocate client memory");
        return NULL;
    }

    memset(client, 0, sizeof(UbusClient));

    // Set timeout
    client->timeout_ms = timeout_ms > 0 ? timeout_ms : UBUS_CLIENT_DEFAULT_TIMEOUT;

    // Connect to UBUS
    client->ctx = ubus_connect(NULL);
    if (!client->ctx) {
        console_error(&csl, "Failed to connect to UBUS");
        free(client);
        return NULL;
    }

    client->connected = true;
    console_info(&csl, "UBUS client initialized successfully");

    return client;
}

// Cleanup UBUS client
void ubus_client_cleanup(UbusClient *client) {
    if (!client) {
        return;
    }

    console_debug(&csl, "Cleaning up UBUS client");

    if (client->ctx) {
        ubus_free(client->ctx);
    }

    free(client);
    console_info(&csl, "UBUS client cleanup complete");
}

// Check if client is connected
bool ubus_client_is_connected(UbusClient *client) {
    if (!client || !client->ctx) {
        return false;
    }

    // Try a simple operation to check connectivity
    uint32_t id;
    int ret = ubus_lookup_id(client->ctx, "system", &id);
    return ret == 0;
}

// Call a UBUS method synchronously
UbusResponse *
ubus_client_call(UbusClient *client, const char *service_name, const char *method_name, struct blob_attr *args) {
    if (!client || !client->ctx || !service_name || !method_name) {
        return create_response(false, -EINVAL, "Invalid parameters");
    }

    console_debug(&csl, "Calling UBUS method: %s.%s", service_name, method_name);

    // Look up service ID
    uint32_t service_id;
    int ret = ubus_lookup_id(client->ctx, service_name, &service_id);
    if (ret) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Service '%s' not found", service_name);
        return create_response(false, ret, error_msg);
    }

    // Create response structure
    UbusResponse *response = create_response(false, 0, NULL);
    if (!response) {
        return create_response(false, -ENOMEM, "Memory allocation failed");
    }

    // Make the call
    UbusResponse *response_ptr = response;
    ret =
        ubus_invoke(client->ctx, service_id, method_name, args, sync_call_callback, &response_ptr, client->timeout_ms);

    if (ret) {
        response->success = false;
        response->error_code = ret;
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "UBUS call failed: %s", ubus_strerror(ret));
        response->error_message = strdup(error_msg);
    }

    return response;
}

// Call a UBUS method with JSON arguments
UbusResponse *
ubus_client_call_json(UbusClient *client, const char *service_name, const char *method_name, const char *json_args) {
    struct blob_attr *args = NULL;

    if (json_args) {
        args = ubus_client_json_to_blob(json_args);
        if (!args) {
            return create_response(false, -EINVAL, "Invalid JSON arguments");
        }
    }

    UbusResponse *response = ubus_client_call(client, service_name, method_name, args);

    if (args) {
        free(args);
    }

    return response;
}

// Call a UBUS method asynchronously
int ubus_client_call_async(UbusClient *client,
                           const char *service_name,
                           const char *method_name,
                           struct blob_attr *args,
                           UbusCallback callback,
                           void *user_data) {
    if (!client || !client->ctx || !service_name || !method_name) {
        return -EINVAL;
    }

    console_debug(&csl, "Calling UBUS method async: %s.%s", service_name, method_name);

    // Look up service ID
    uint32_t service_id;
    int ret = ubus_lookup_id(client->ctx, service_name, &service_id);
    if (ret) {
        console_error(&csl, "Service '%s' not found", service_name);
        return ret;
    }

    // Create async context
    AsyncCallContext *ctx = malloc(sizeof(AsyncCallContext));
    if (!ctx) {
        return -ENOMEM;
    }

    ctx->callback = callback;
    ctx->user_data = user_data;
    ctx->client = client;
    ctx->completed = false;

    // Make async call using synchronous call for now (async API is complex)
    UbusResponse *response = ubus_client_call(client, service_name, method_name, args);

    // Call the callback immediately
    if (callback) {
        callback(response, user_data);
    }

    // Cleanup
    ubus_response_free(response);
    free(ctx);

    return 0;
}

// List available UBUS services
UbusResponse *ubus_client_list_services(UbusClient *client) {
    if (!client || !client->ctx) {
        return create_response(false, -EINVAL, "Invalid client");
    }

    console_debug(&csl, "Listing UBUS services");

    // Use ubus list command equivalent
    return ubus_client_call(client, "ubus", "list", NULL);
}

// Get methods for a specific service
UbusResponse *ubus_client_get_service_methods(UbusClient *client, const char *service_name) {
    if (!client || !service_name) {
        return create_response(false, -EINVAL, "Invalid parameters");
    }

    console_debug(&csl, "Getting methods for service: %s", service_name);

    // Create arguments for the list call
    struct blob_buf args;
    blob_buf_init(&args, 0);
    blobmsg_add_string(&args, "path", service_name);

    UbusResponse *response = ubus_client_call(client, "ubus", "list", args.head);

    blob_buf_free(&args);
    return response;
}

// Send a simple ping to a service
bool ubus_client_ping_service(UbusClient *client, const char *service_name) {
    if (!client || !service_name) {
        return false;
    }

    console_debug(&csl, "Pinging service: %s", service_name);

    // Try to call ping method if available, otherwise just lookup
    UbusResponse *response = ubus_client_call(client, service_name, "ping", NULL);
    bool result = response && response->success;

    if (!result) {
        // If ping method doesn't exist, just check if service exists
        uint32_t service_id;
        int ret = ubus_lookup_id(client->ctx, service_name, &service_id);
        result = (ret == 0);
    }

    if (response) {
        ubus_response_free(response);
    }

    return result;
}

// Free UBUS response structure
void ubus_response_free(UbusResponse *response) {
    if (!response) {
        return;
    }

    if (response->error_message) {
        free(response->error_message);
    }

    if (response->data) {
        free(response->data);
    }

    if (response->json_response) {
        free(response->json_response);
    }

    free(response);
}

// Get string value from UBUS response
const char *ubus_response_get_string(UbusResponse *response, const char *key) {
    if (!response || !response->data || !key) {
        return NULL;
    }

    struct blob_attr *attr;
    int rem;
    blobmsg_for_each_attr(attr, response->data, rem) {
        if (blobmsg_type(attr) == BLOBMSG_TYPE_STRING && strcmp(blobmsg_name(attr), key) == 0) {
            return blobmsg_get_string(attr);
        }
    }
    return NULL;
}

// Get integer value from UBUS response
int ubus_response_get_int(UbusResponse *response, const char *key, int default_value) {
    if (!response || !response->data || !key) {
        return default_value;
    }

    struct blob_attr *attr;
    int rem;
    blobmsg_for_each_attr(attr, response->data, rem) {
        if (blobmsg_type(attr) == BLOBMSG_TYPE_INT32 && strcmp(blobmsg_name(attr), key) == 0) {
            return blobmsg_get_u32(attr);
        }
    }
    return default_value;
}

// Get boolean value from UBUS response
bool ubus_response_get_bool(UbusResponse *response, const char *key, bool default_value) {
    if (!response || !response->data || !key) {
        return default_value;
    }

    struct blob_attr *attr;
    int rem;
    blobmsg_for_each_attr(attr, response->data, rem) {
        if (blobmsg_type(attr) == BLOBMSG_TYPE_INT8 && strcmp(blobmsg_name(attr), key) == 0) {
            return blobmsg_get_bool(attr);
        }
    }
    return default_value;
}

// Convert UBUS response to JSON string
char *ubus_response_to_json(UbusResponse *response) {
    if (!response) {
        return NULL;
    }

    if (response->json_response) {
        return strdup(response->json_response);
    }

    if (!response->data) {
        return NULL;
    }

    return blobmsg_format_json(response->data, true);
}

// Helper function to create blob_attr from JSON string
struct blob_attr *ubus_client_json_to_blob(const char *json_str) {
    if (!json_str) {
        return NULL;
    }

    struct blob_buf buf;
    blob_buf_init(&buf, 0);

    if (!blobmsg_add_json_from_string(&buf, json_str)) {
        blob_buf_free(&buf);
        return NULL;
    }

    // Clone the buffer head
    struct blob_attr *attr = blob_memdup(buf.head);
    blob_buf_free(&buf);

    return attr;
}

// Helper function to create JSON string from blob_attr
char *ubus_client_blob_to_json(struct blob_attr *attr) {
    if (!attr) {
        return NULL;
    }

    return blobmsg_format_json(attr, true);
}