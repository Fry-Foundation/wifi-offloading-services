#include "ubus.h"        
#include "core/console.h"
#include <libubus.h>
#include <libubox/blobmsg.h>
#include <libubox/blobmsg_json.h>  
#include <string.h>

static Console csl = {
    .topic = "ubus-client",  
};

// Structure to hold UBUS response data
struct token_response_data {
    struct blob_buf buf;
    bool received;
};

// UBUS sync response callback for access token
static void ubus_sync_response_cb(struct ubus_request *req, int type, struct blob_attr *msg) {
    struct token_response_data *data = (struct token_response_data *)req->priv;

    console_debug(&csl, "Sync response callback: type=%d, msg=%p, data=%p", type, msg, data);

    if (type == UBUS_MSG_DATA && msg && data) {
        blob_buf_init(&data->buf, 0);
        blob_put_raw(&data->buf, blob_data(msg), blob_len(msg));
        data->received = true;
        
        console_debug(&csl, "Response message copied to buffer");
    }
}

// Check if UBUS is available for token requests
bool ubus_is_available_for_tokens(void) {
    struct ubus_context *ctx = ubus_connect(NULL);
    if (!ctx) {
        console_debug(&csl, "Failed to connect to UBUS daemon");
        return false;
    }
    
    uint32_t id;
    int ret = ubus_lookup_id(ctx, "wayru-agent", &id);
    ubus_free(ctx);
    
    if (ret != 0) {
        console_debug(&csl, "wayru-agent object not found in UBUS (not ready yet)");
        return false;
    }
    
    console_debug(&csl, "UBUS connectivity to wayru-agent confirmed");
    return true;
}

// Get access token from wayru-agent via UBUS (synchronous)
int ubus_get_access_token_sync(char *token_buf, size_t token_size, time_t *expiry) {
    struct ubus_context *ctx = NULL;
    uint32_t id;
    struct blob_buf b = {};
    struct token_response_data response_data = {.received = false};
    int ret;

    if (!token_buf || token_size < 2 || !expiry) {
        console_error(&csl, "Invalid parameters for token request");
        return -1;
    }

    // UBUS client initialization
    console_debug(&csl, "Connecting to UBUS for token request...");
    ctx = ubus_connect(NULL);
    if (!ctx) {
        console_error(&csl, "Failed to connect to UBUS");
        return -1;
    }

    // Look up wayru-agent object
    ret = ubus_lookup_id(ctx, "wayru-agent", &id);
    if (ret != 0) {
        console_error(&csl, "Failed to find wayru-agent object: %s", ubus_strerror(ret));
        ubus_free(ctx);
        return -1;
    }

    console_debug(&csl, "Found wayru-agent object with id: %u", id);

    // Prepare request
    blob_buf_init(&b, 0);

    // Request token from wayru-agent via ubus
    console_debug(&csl, "Requesting access token from wayru-agent...");
    ret = ubus_invoke(ctx, id, "get_access_token", b.head, ubus_sync_response_cb, &response_data, 5000);

    blob_buf_free(&b);

    if (ret != 0) {
        console_error(&csl, "Failed to get access token from wayru-agent: %s", ubus_strerror(ret));
        ubus_free(ctx);
        return -1;
    }

    if (!response_data.received || !response_data.buf.head) {
        console_error(&csl, "No response received from wayru-agent");
        ubus_free(ctx);
        return -1;
    }

    char *json_str = blobmsg_format_json(response_data.buf.head, true);
    if (json_str) {
        console_debug(&csl, "Raw token response: %s", json_str);
        free(json_str);
    }

    // Parse the response
    enum { TOKEN_FIELD, ISSUED_AT_FIELD, EXPIRES_AT_FIELD, VALID_FIELD, __TOKEN_MAX };

    static const struct blobmsg_policy token_policy[__TOKEN_MAX] = {
        [TOKEN_FIELD] = {.name = "token", .type = BLOBMSG_TYPE_STRING},
        [ISSUED_AT_FIELD] = {.name = "issued_at", .type = BLOBMSG_TYPE_INT64},
        [EXPIRES_AT_FIELD] = {.name = "expires_at", .type = BLOBMSG_TYPE_INT64},
        [VALID_FIELD] = {.name = "valid", .type = BLOBMSG_TYPE_INT8},
    };

    struct blob_attr *tb[__TOKEN_MAX];

    if (blobmsg_parse(token_policy, __TOKEN_MAX, tb, blobmsg_data(response_data.buf.head),
                      blobmsg_len(response_data.buf.head)) != 0) {
        console_error(&csl, "Failed to parse token response");
        blob_buf_free(&response_data.buf);
        ubus_free(ctx);
        return -1;
    }

    console_debug(&csl, "Token field present: %s", tb[TOKEN_FIELD] ? "yes" : "no");
    console_debug(&csl, "Issued_at field present: %s", tb[ISSUED_AT_FIELD] ? "yes" : "no");
    console_debug(&csl, "Expires_at field present: %s", tb[EXPIRES_AT_FIELD] ? "yes" : "no");
    console_debug(&csl, "Valid field present: %s", tb[VALID_FIELD] ? "yes" : "no");

    // Check if all required fields are present
    if (!tb[TOKEN_FIELD] || !tb[EXPIRES_AT_FIELD] || !tb[VALID_FIELD]) {
        console_error(&csl, "Missing required fields in token response");
        blob_buf_free(&response_data.buf);
        ubus_free(ctx);
        return -1;
    }

    // Check if token is valid
    uint8_t valid = blobmsg_get_u8(tb[VALID_FIELD]);

    // Log token validity
    console_debug(&csl, "Token valid field: %s", valid ? "true" : "false");
    
    if (!valid) {
        console_error(&csl, "Token marked as invalid by wayru-agent");
        blob_buf_free(&response_data.buf);
        ubus_free(ctx);
        return -1;
    }

    // Extract token
    const char *token = blobmsg_get_string(tb[TOKEN_FIELD]);
    if (!token || strlen(token) == 0) {
        console_error(&csl, "Empty token received from wayru-agent");
        blob_buf_free(&response_data.buf);
        ubus_free(ctx);
        return -1;
    }

    // Log token length
    console_debug(&csl, "Token length: %zu", strlen(token));

    // Copy token to buffer
    size_t token_len = strlen(token);
    if (token_len >= token_size) {
        console_error(&csl, "Token too large for buffer (token: %zu, buffer: %zu)", token_len, token_size);
        blob_buf_free(&response_data.buf);
        ubus_free(ctx);
        return -1;
    }

    strncpy(token_buf, token, token_size - 1);
    token_buf[token_size - 1] = '\0';

    // Extract expiry time
    *expiry = (time_t)blobmsg_get_u64(tb[EXPIRES_AT_FIELD]);

    console_info(&csl, "Successfully retrieved access token from wayru-agent, expires at %ld", *expiry);

    // Cleanup
    blob_buf_free(&response_data.buf);
    ubus_free(ctx);

    return 0;
}