#include "ubus.h"
#include "collect.h"
#include "core/console.h"
#include <libubus.h>
#include <libubox/uloop.h>
#include <libubox/blobmsg.h>
#include <libubox/blobmsg_json.h>
#include <string.h>
#include <unistd.h>

static Console csl = {
    .topic = "ubus",
};

static struct ubus_context *ctx = NULL;
static struct ubus_subscriber subscriber;
static volatile bool connected = false;
static struct uloop_timeout reconnect_timer;
static struct uloop_timeout resubscribe_timer;

// UBUS message policies for syslog events
enum {
    SYSLOG_MSG_PROGRAM,
    SYSLOG_MSG_MESSAGE,
    SYSLOG_MSG_FACILITY,
    SYSLOG_MSG_PRIORITY,
    SYSLOG_MSG_TIMESTAMP,
    __SYSLOG_MSG_MAX
};

static const struct blobmsg_policy syslog_policy[__SYSLOG_MSG_MAX] = {
    [SYSLOG_MSG_PROGRAM] = { .name = "program", .type = BLOBMSG_TYPE_STRING },
    [SYSLOG_MSG_MESSAGE] = { .name = "message", .type = BLOBMSG_TYPE_STRING },
    [SYSLOG_MSG_FACILITY] = { .name = "facility", .type = BLOBMSG_TYPE_STRING },
    [SYSLOG_MSG_PRIORITY] = { .name = "priority", .type = BLOBMSG_TYPE_STRING },
    [SYSLOG_MSG_TIMESTAMP] = { .name = "timestamp", .type = BLOBMSG_TYPE_INT64 },
};

/**
 * Quick filter to determine if we should process this log entry
 * @param program Program name
 * @param message Log message
 * @return true if should process, false to skip
 */
static bool should_process_log(const char *program, const char *message) {
    // Skip empty messages
    if (!message || strlen(message) == 0) {
        return false;
    }
    
    // Skip kernel messages for now (too noisy)
    if (program && strcmp(program, "kernel") == 0) {
        return false;
    }
    
    // Skip debug messages in non-dev mode
    if (message && strstr(message, "DEBUG:") == message) {
        return false;
    }
    
    // Skip very short messages (likely noise)
    if (strlen(message) < 3) {
        return false;
    }
    
    // Skip messages from collector itself to avoid loops
    if (program && (strcmp(program, "collector") == 0 || 
                   strstr(program, "wayru-collector") != NULL)) {
        return false;
    }
    
    return true;
}

/**
 * UBUS notification callback for syslog events
 * This is called directly from uloop, so must be fast
 */
static int syslog_notify_cb(struct ubus_context *ctx, struct ubus_object *obj,
                           struct ubus_request_data *req, const char *method,
                           struct blob_attr *msg) {
    struct blob_attr *tb[__SYSLOG_MSG_MAX];
    const char *program = "";
    const char *message = "";
    const char *facility = "";
    const char *priority = "";
    
    (void)ctx;
    (void)obj;
    (void)req;
    (void)method;
    
    // Parse the message
    blobmsg_parse(syslog_policy, __SYSLOG_MSG_MAX, tb, blob_data(msg), blob_len(msg));
    
    // Extract fields
    if (tb[SYSLOG_MSG_PROGRAM]) {
        program = blobmsg_get_string(tb[SYSLOG_MSG_PROGRAM]);
    }
    if (tb[SYSLOG_MSG_MESSAGE]) {
        message = blobmsg_get_string(tb[SYSLOG_MSG_MESSAGE]);
    }
    if (tb[SYSLOG_MSG_FACILITY]) {
        facility = blobmsg_get_string(tb[SYSLOG_MSG_FACILITY]);
    }
    if (tb[SYSLOG_MSG_PRIORITY]) {
        priority = blobmsg_get_string(tb[SYSLOG_MSG_PRIORITY]);
    }
    
    // Quick filter - this must be fast to avoid blocking uloop
    if (!should_process_log(program, message)) {
        return 0;
    }
    
    // Enqueue for processing - this is non-blocking in single-threaded mode
    int ret = collect_enqueue_log(program, message, facility, priority);
    if (ret < 0) {
        // Only log debug message if in development mode to avoid spam
        console_debug(&csl, "Failed to enqueue log from %s: %d", program, ret);
    }
    
    // Check if we need urgent batch processing
    uint32_t queue_size, dropped_count;
    if (collect_get_stats(&queue_size, &dropped_count) == 0) {
        if (queue_size >= 400) { // 80% of reduced queue size
            collect_force_batch_processing();
        }
    }
    
    return 0;
}

/**
 * UBUS connection lost callback
 */
static void ubus_connection_lost_cb(struct ubus_context *ctx) {
    (void)ctx;
    console_warn(&csl, "UBUS connection lost, will attempt reconnect");
    connected = false;
    
    // Schedule reconnection attempt
    uloop_timeout_set(&reconnect_timer, 5000); // Try again in 5 seconds
}

/**
 * Timer callback to attempt UBUS reconnection
 */
static void reconnect_timer_cb(struct uloop_timeout *timeout) {
    (void)timeout;
    
    if (connected) {
        return; // Already reconnected
    }
    
    console_info(&csl, "Attempting to reconnect to UBUS...");
    
    // Clean up old context
    if (ctx) {
        ubus_free(ctx);
        ctx = NULL;
    }
    
    // Try to reconnect
    if (ubus_init() == 0) {
        console_info(&csl, "UBUS reconnected successfully");
    } else {
        console_warn(&csl, "UBUS reconnection failed, will retry");
        uloop_timeout_set(&reconnect_timer, 10000); // Try again in 10 seconds
    }
}

/**
 * Timer callback to check and resubscribe to log service
 */
static void resubscribe_timer_cb(struct uloop_timeout *timeout) {
    (void)timeout;
    
    if (!connected || !ctx) {
        return;
    }
    
    // Check if log service is available and resubscribe if needed
    uint32_t id;
    int ret = ubus_lookup_id(ctx, "log", &id);
    if (ret == 0) {
        ret = ubus_subscribe(ctx, &subscriber, id);
        if (ret == 0) {
            console_debug(&csl, "Resubscribed to log service");
        } else {
            console_debug(&csl, "Failed to resubscribe: %s", ubus_strerror(ret));
        }
    }
    
    // Schedule next check
    uloop_timeout_set(&resubscribe_timer, 60000); // Check every minute
}

int ubus_init(void) {
    int ret;
    
    console_info(&csl, "Initializing UBUS connection for single-core mode...");
    
    // Connect to UBUS
    ctx = ubus_connect(NULL);
    if (!ctx) {
        console_error(&csl, "Failed to connect to UBUS");
        return -1;
    }
    
    // Set connection lost callback
    ctx->connection_lost = ubus_connection_lost_cb;
    
    // Initialize subscriber
    subscriber.cb = syslog_notify_cb;
    ret = ubus_register_subscriber(ctx, &subscriber);
    if (ret) {
        console_error(&csl, "Failed to register UBUS subscriber: %s", ubus_strerror(ret));
        ubus_free(ctx);
        ctx = NULL;
        return ret;
    }
    
    // Add UBUS context to uloop for integration
    ubus_add_uloop(ctx);
    
    // Subscribe to syslog events
    uint32_t id;
    ret = ubus_lookup_id(ctx, "log", &id);
    if (ret) {
        console_warn(&csl, "Failed to find log service: %s (will retry)", ubus_strerror(ret));
        // Don't fail completely - the log service might start later
    } else {
        ret = ubus_subscribe(ctx, &subscriber, id);
        if (ret) {
            console_warn(&csl, "Failed to subscribe to log service: %s", ubus_strerror(ret));
        } else {
            console_info(&csl, "Successfully subscribed to syslog events");
        }
    }
    
    // Set up timers for reconnection and resubscription
    reconnect_timer.cb = reconnect_timer_cb;
    resubscribe_timer.cb = resubscribe_timer_cb;
    
    // Start resubscribe timer
    uloop_timeout_set(&resubscribe_timer, 60000); // First check in 1 minute
    
    connected = true;
    
    console_info(&csl, "UBUS initialization complete (single-core mode)");
    return 0;
}

int ubus_start_loop(void) {
    // In single-core mode, we don't start a separate loop
    // UBUS events are handled through the main uloop
    console_info(&csl, "UBUS integrated with main event loop");
    return 0;
}

void ubus_cleanup(void) {
    console_info(&csl, "Cleaning up UBUS connection...");
    
    connected = false;
    
    // Cancel timers
    uloop_timeout_cancel(&reconnect_timer);
    uloop_timeout_cancel(&resubscribe_timer);
    
    if (ctx) {
        // Unregister subscriber
        ubus_unregister_subscriber(ctx, &subscriber);
        
        // Free context
        ubus_free(ctx);
        ctx = NULL;
    }
    
    console_info(&csl, "UBUS cleanup complete");
}

bool ubus_is_connected(void) {
    return connected && (ctx != NULL);
}