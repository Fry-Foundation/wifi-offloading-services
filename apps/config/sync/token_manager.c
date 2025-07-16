#include "token_manager.h"
#include "sync.h"
#include "ubus.h"
#include "core/console.h"
#include <string.h>
#include <time.h>

#define MAX_NETWORK_FAILURES 3

static Console csl = {
    .topic = "token-mgr",
};

// Check if the current access token is still valid
bool sync_is_token_valid(struct ConfigSyncContext *context) {
    if (!context || !context->token_initialized) {
        return false;
    }
    
    if (strlen(context->access_token) == 0) {
        return false;
    }
    
    time_t now = time(NULL);
    if (now >= context->token_expiry) {
        console_debug(&csl, "Token expired: now=%ld, expiry=%ld", now, context->token_expiry);
        return false;
    }
    
    return true;
}

// Refresh access token
int sync_refresh_access_token(struct ConfigSyncContext *context) {
    if (!context) {
        console_error(&csl, "Invalid context for token refresh");
        return -1;
    }

    console_info(&csl, "Access token expired or invalid, refreshing...");
    console_debug(&csl, "Refreshing access token via UBUS...");

    // Check if UBUS is available for token requests
    if (!ubus_is_available_for_tokens()) {
        console_debug(&csl, "UBUS not available for token requests (wayru-agent not ready)");
        return -1; 
    }

    char token_buffer[256];
    time_t expiry;

    // Use existing UBUS function to get access token
    int ret = ubus_get_access_token_sync(token_buffer, sizeof(token_buffer), &expiry);
    if (ret < 0) {
        console_error(&csl, "Failed to refresh access token via UBUS");
        context->consecutive_http_failures++;
        
        // Disable requests on too many failures
        if (context->consecutive_http_failures >= MAX_NETWORK_FAILURES) {
            console_warn(&csl, "Too many token failures (%d), disabling requests", context->consecutive_http_failures);
            sync_set_request_acceptance(context, false);
        }
        
        return ret;
    }

    // Update token in context
    strncpy(context->access_token, token_buffer, sizeof(context->access_token) - 1);
    context->access_token[sizeof(context->access_token) - 1] = '\0';
    context->token_expiry = expiry;
    context->token_initialized = true;
    context->consecutive_http_failures = 0;  

    if (!context->accept_requests) {
        console_info(&csl, "Enabling request acceptance - token available and network healthy");
    }

    // Enable request acceptance when we have a valid token
    sync_set_request_acceptance(context, true);

    console_info(&csl, "Access token refreshed successfully via UBUS");
    return 0;
}

// Get current access token for HTTP requests
const char *sync_get_current_token(struct ConfigSyncContext *context) {
    if (!context || !context->token_initialized) {
        return NULL;
    }
    
    if (!sync_is_token_valid(context)) {
        return NULL;
    }
    
    return context->access_token;
}

// Set whether the sync should accept new HTTP requests
void sync_set_request_acceptance(struct ConfigSyncContext *context, bool accept) {
    if (!context) {
        return;
    }
    
    if (context->accept_requests != accept) {
        console_info(&csl, "Request acceptance %s", accept ? "enabled" : "disabled");
        context->accept_requests = accept;
    }
}

// Check if the sync should accept new HTTP requests
bool sync_should_accept_requests(struct ConfigSyncContext *context) {
    if (!context) {
        return false;
    }
    
    return context->accept_requests;
}

// Report HTTP request failure for network monitoring
void sync_report_http_failure(struct ConfigSyncContext *context, int error_code) {
    if (!context) {
        return;
    }
    
    context->consecutive_http_failures++;
    console_debug(&csl, "HTTP failure reported: code=%d, consecutive=%d", 
                 error_code, context->consecutive_http_failures);
    
    // Disable requests after too many failures
    if (context->consecutive_http_failures >= MAX_NETWORK_FAILURES && context->accept_requests) {
        console_warn(&csl, "Too many HTTP failures (%d), disabling requests", 
                    context->consecutive_http_failures);
        sync_set_request_acceptance(context, false);
    }
}

// Report successful HTTP request to reset failure counter
void sync_report_http_success(struct ConfigSyncContext *context) {
    if (!context) {
        return;
    }
    
    if (context->consecutive_http_failures > 0) {
        console_debug(&csl, "HTTP success - resetting failure counter (was %d)", 
                     context->consecutive_http_failures);
        context->consecutive_http_failures = 0;
    }
    
    // Re-enable requests on success if we have a valid token
    if (!context->accept_requests && sync_is_token_valid(context)) {
        console_info(&csl, "HTTP success and token valid, re-enabling requests");
        sync_set_request_acceptance(context, true);
    }
}