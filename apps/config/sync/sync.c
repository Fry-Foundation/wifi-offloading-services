#include "sync.h"
#include "core/console.h"
#include "http/http-requests.h"  
#include "config.h"
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "renderer/renderer.h"
#include <sys/stat.h>
#include <time.h>        

static Console csl = {.topic = "config-sync"};

// Structure to track which services need restart
typedef struct {
    bool wireless;
    bool wayru_agent;
    bool wayru_collector;
    bool wayru_config;
    bool opennds;  
} ServiceRestartNeeds;


// Analyze which services need restart based on configuration changes
// Uses granular hash comparison per service section
static ServiceRestartNeeds analyze_restart_needs(const char *json, bool dev_mode) {
    ServiceRestartNeeds needs = {false, false, false, false, false}; 
    
    // Check each service section for changes using hash comparison
    needs.wireless = config_affects_wireless(json, dev_mode);
    needs.wayru_agent = config_affects_wayru_agent(json, dev_mode);
    needs.wayru_collector = config_affects_wayru_collector(json, dev_mode);
    needs.wayru_config = config_affects_wayru_config(json, dev_mode);
    needs.opennds = config_affects_opennds(json, dev_mode);
    
    console_debug(&csl, "Restart analysis - wireless: %s, agent: %s, collector: %s, config: %s, opennds: %s",
                 needs.wireless ? "YES" : "no",
                 needs.wayru_agent ? "YES" : "no", 
                 needs.wayru_collector ? "YES" : "no",
                 needs.wayru_config ? "YES" : "no",
                 needs.opennds ? "YES" : "no");
    
    return needs;
}

// Development mode: Log what would be restarted without actually doing it
static void handle_dev_mode_restart(const ServiceRestartNeeds *needs) {
    console_info(&csl, "Development mode: showing what would be restarted");
    
    if (needs->wireless) {
        console_info(&csl, "Would reload: wifi configuration");
    }
    if (needs->wayru_collector) {
        console_info(&csl, "Would restart: wayru-collector service");
    }
    if (needs->wayru_agent) {
        console_info(&csl, "Would restart: wayru-agent service");
    }
    if (needs->wayru_config) {
        console_info(&csl, "Would reload: wayru-config configuration (via procd reload)");
    }
    if (needs->opennds) { 
        console_info(&csl, "Would restart: opennds service");
    }
    
    if (!needs->wireless && !needs->wayru_collector && !needs->wayru_agent && !needs->wayru_config && !needs->opennds) {
        console_info(&csl, "No services need restart");
    }
}

// Production mode: Actually restart services 
static int restart_services_production(const ServiceRestartNeeds *needs) {
    console_info(&csl, "Applying configuration changes to services...");
    int total_errors = 0;
    
    // 1. Reload WiFi first 
    if (needs->wireless) {
        console_info(&csl, "Reloading WiFi configuration...");
        int result = system("wifi reload");
        if (WEXITSTATUS(result) == 0) {
            console_info(&csl, "WiFi configuration reloaded successfully");
        } else {
            console_warn(&csl, "WiFi reload returned code %d", WEXITSTATUS(result));
            total_errors++;
        }
        sleep(1); // Allow WiFi to stabilize
    }

    // 2. Restart OpenNDS 
    if (needs->opennds) {  
        console_info(&csl, "Restarting OpenNDS...");
        int result = system("/etc/init.d/opennds restart");
        if (WEXITSTATUS(result) == 0) {
            console_info(&csl, "OpenNDS restarted successfully");
        } else {
            console_warn(&csl, "OpenNDS restart failed with code %d", WEXITSTATUS(result));
            total_errors++;
        }
        sleep(2); // Allow OpenNDS to initialize
    }        
    
    // 3. Restart wayru-collector 
    if (needs->wayru_collector) {
        console_info(&csl, "Restarting wayru-collector...");
        int result = system("/etc/init.d/wayru-collector reload");
        if (WEXITSTATUS(result) == 0) {
            console_info(&csl, "wayru-collector reloaded successfully");
        } else {
            console_warn(&csl, "wayru-collector reload failed, trying restart...");
            result = system("/etc/init.d/wayru-collector restart");
            if (WEXITSTATUS(result) == 0) {
                console_info(&csl, "wayru-collector restarted successfully");
            } else {
                console_error(&csl, "wayru-collector restart failed with code %d", WEXITSTATUS(result));
                total_errors++;
            }
        }
        sleep(2); // Allow collector to initialize
    }

    // 4. Restart wayru-agent 
    if (needs->wayru_agent) {
        console_info(&csl, "Restarting wayru-agent...");
        int result = system("/etc/init.d/wayru-agent reload");
        if (WEXITSTATUS(result) == 0) {
            console_info(&csl, "wayru-agent reloaded successfully");
        } else {
            console_warn(&csl, "wayru-agent reload failed, trying restart...");
            result = system("/etc/init.d/wayru-agent restart");
            if (WEXITSTATUS(result) == 0) {
                console_info(&csl, "wayru-agent restarted successfully");
            } else {
                console_error(&csl, "wayru-agent restart failed with code %d", WEXITSTATUS(result));
                total_errors++;
            }
        }
        sleep(2); // Allow agent to initialize
    }

    // 5. Reload wayru-config last 
    if (needs->wayru_config) {
        console_info(&csl, "wayru-config configuration changed, triggering reload...");
        int result = system("/etc/init.d/wayru-config reload");
        if (WEXITSTATUS(result) == 0) {
            console_info(&csl, "wayru-config reload triggered successfully");
        } else {
            console_warn(&csl, "wayru-config reload failed with code %d, may need manual restart", WEXITSTATUS(result));
            total_errors++;
        }
    }

    // Report final status
    if (total_errors == 0) {
        console_info(&csl, "All service operations completed successfully");
    } else {
        console_warn(&csl, "Service restart completed with %d errors", total_errors);
    }
    
    return total_errors;
}

/**
 * Fetch device configuration from remote endpoint
 * Returns allocated JSON string or NULL on failure
 */
char *fetch_device_config_json(const char *endpoint, ConfigSyncContext *context) {
    if (!endpoint || strlen(endpoint) == 0) {
        console_error(&csl, "Missing config endpoint");
        return NULL;
    }

    // Check should requests be accepted
    if (!sync_should_accept_requests(context)) {
        console_debug(&csl, "Rejecting config request - request acceptance disabled");
        return NULL;
    }

    // Get cached access token
    const char *access_token = sync_get_current_token(context);
    if (!access_token) {
        console_warn(&csl, "No valid access token available, aborting config request");
        sync_report_http_failure(context, -1);
        return NULL;
    }

    console_debug(&csl, "Making config request with Bearer token");
    
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    HttpGetOptions options = {
        .url = endpoint,
        .bearer_token = access_token,  
    };

    HttpResult result = http_get(&options);

    // Calculate request duration
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double duration_ms = (end_time.tv_sec - start_time.tv_sec) * 1000.0 + 
                        (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;

    if (result.is_error || !result.response_buffer) {
        console_warn(&csl, "Config fetch failed: %s - took %.2f ms", 
                    result.error, duration_ms);
        sync_report_http_failure(context, result.http_status_code);
        
        if (result.response_buffer) {
            free(result.response_buffer);
        }
        return NULL;
    }

    // Check HTTP status codes
    if (result.http_status_code >= 200 && result.http_status_code < 300) {
        console_info(&csl, "Config request successful (code: %ld) - took %.2f ms", 
                    result.http_status_code, duration_ms);
        sync_report_http_success(context);
        
        size_t json_length = strlen(result.response_buffer);
        console_info(&csl, "Received config JSON (%zu bytes): %.200s%s", 
                     json_length, 
                     result.response_buffer,
                     json_length > 200 ? "..." : "");

        return result.response_buffer;  
        
    } else if (result.http_status_code == 401) {
        console_warn(&csl, "Config request failed with 401 Unauthorized, will refresh token - took %.2f ms", 
                    duration_ms);
        sync_report_http_failure(context, result.http_status_code);
    } else {
        console_warn(&csl, "Config request failed with code: %ld - took %.2f ms", 
                    result.http_status_code, duration_ms);
        sync_report_http_failure(context, result.http_status_code);
    }

    // Cleanup on error
    if (result.response_buffer) {
        free(result.response_buffer);
    }
    return NULL;
}

// Main periodic task: fetch config, detect changes, apply updates

static void config_sync_task(void *ctx) {
    ConfigSyncContext *context = (ConfigSyncContext *)ctx;
    
    console_debug(&csl, "Executing config sync task");

    // Check should requests be accepted
    if (!sync_should_accept_requests(context)) {
        console_debug(&csl, "Skipping config sync - requests disabled");
        return;
    }

    if (!sync_is_token_valid(context)) {
        console_info(&csl, "Access token expired, attempting refresh...");
        int ret = sync_refresh_access_token(context);
        if (ret < 0) {
            console_warn(&csl, "Failed to refresh token, skipping this cycle");
            return;
        }
    }
    
    // 1. Fetch configuration JSON from endpoint
    char *json = fetch_device_config_json(context->endpoint, context);
    if (!json) {
        console_warn(&csl, "No configuration received, skipping this cycle");
        return;
    }
    
    console_info(&csl, "Configuration received, analyzing changes...");
    
    // 2. Analyze what services need restart using hash comparison
    ServiceRestartNeeds needs = analyze_restart_needs(json, context->dev_mode);
    
    // 3. Skip if no changes detected
    if (!needs.wireless && !needs.wayru_agent && !needs.wayru_collector && !needs.wayru_config && !needs.opennds) {
        console_info(&csl, "No configuration changes detected, skipping application");
        free(json);
        return;
    }
    
    console_info(&csl, "Configuration changes detected, applying updates...");
    
    // 4. Apply configuration to UCI
    if (apply_config_without_restarts(json, context->dev_mode) == 0) {
        console_info(&csl, "Configuration applied successfully");
        
        // 5. Handle service restarts based on mode
        if (context->dev_mode) {
            handle_dev_mode_restart(&needs);
        } else {
            restart_services_production(&needs);
        }
        
    } else {
        console_error(&csl, "Failed to apply configuration");
    }
    
    free(json);
}

//Initialize and start the configuration sync service
// Sets up periodic task and renderer configuration
 
ConfigSyncContext *start_config_sync_service(const char *endpoint, 
                                            uint32_t initial_delay_ms,
                                            uint32_t interval_ms, 
                                            bool dev_mode) {
    // Allocate context structure
    ConfigSyncContext *context = (ConfigSyncContext *)malloc(sizeof(ConfigSyncContext));
    if (!context) {
        console_error(&csl, "Failed to allocate memory for config sync context");
        return NULL;
    }

    // Initialize context
    context->endpoint = endpoint;
    context->dev_mode = dev_mode;
    context->current_interval_ms = interval_ms;  
    
    // Initialize token management
    memset(context->access_token, 0, sizeof(context->access_token));
    context->token_expiry = 0;
    context->token_initialized = false;
    context->accept_requests = false;  // Start disabled until token is acquired
    context->consecutive_http_failures = 0;
    
    // Configure renderer for appropriate mode
    set_renderer_dev_mode(dev_mode);
    
    console_info(&csl, "Section hashes will be stored in: %s", 
                 dev_mode ? "./scripts/dev/hashes" : "/etc/wayru-config/hashes");
    
    console_info(&csl, "Starting config sync service with initial delay %u ms, interval %u ms", 
                 initial_delay_ms, interval_ms);
    
    // Try to get initial token
    console_info(&csl, "Attempting to acquire initial access token...");
    int token_ret = sync_refresh_access_token(context);
    if (token_ret == 0) {
        console_info(&csl, "Initial access token acquired successfully");
    } else {
        console_warn(&csl, "Failed to acquire initial token, will retry during operation");
    }
    
    // Schedule the periodic task
    context->task_id = schedule_repeating(initial_delay_ms, interval_ms, config_sync_task, context);
    if (context->task_id == 0) {
        console_error(&csl, "Failed to schedule config sync task");
        free(context);
        return NULL;
    }

    console_debug(&csl, "Successfully scheduled config sync task with ID %u", context->task_id);
    return context;
}


//Clean up configuration sync context and cancel scheduled task

void clean_config_sync_context(ConfigSyncContext *context) {
    if (!context) {
        return;
    }
    
    console_info(&csl, "Cleaning config sync context...");
    
    // Cancel scheduled task
    if (context->task_id != 0) {
        console_debug(&csl, "Cancelling sync task ID: %u", context->task_id);
        cancel_task(context->task_id);
        context->task_id = 0;
    }
    
    // Limpiar token en memoria
    if (context->token_initialized) {
        console_debug(&csl, "Clearing access token from memory");
        memset(context->access_token, 0, sizeof(context->access_token));
        context->token_initialized = false;
        context->token_expiry = 0;
    }
    
    // Reset flags
    context->accept_requests = false;
    context->consecutive_http_failures = 0;
    
    // Free context memory
    console_debug(&csl, "Freeing sync context memory");
    free(context);
    
    console_info(&csl, "Config sync context cleaned successfully");
}




