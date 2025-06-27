#include "sync.h"
#include "core/console.h"
#include "http/http-requests.h"
#include "config.h"
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include "renderer/renderer.h"

static Console csl = {.topic = "config-sync"};



char *fetch_device_config_json(const char *endpoint) {
    if (!endpoint || strlen(endpoint) == 0) {
        console_error(&csl, "Missing config endpoint");
        return NULL;
    }

    HttpGetOptions options = {
        .url = endpoint,
        //.bearer_token 
    };

    HttpResult result = http_get(&options);

    if (result.is_error || !result.response_buffer) {
        console_error(&csl, "Config fetch failed: %s", result.error);
        if (result.response_buffer) free(result.response_buffer);
        return NULL;
    }

    // Debug: mostrar tamaño del JSON recibido
    size_t json_length = strlen(result.response_buffer);
    console_info(&csl, "Received config JSON (%zu bytes): %.200s%s", 
                 json_length, 
                 result.response_buffer,
                 json_length > 200 ? "..." : "");

    return result.response_buffer;  
}

static void config_sync_task(void *ctx) {
    ConfigSyncContext *context = (ConfigSyncContext *)ctx;
    
    console_debug(&csl, "Executing config sync task");
    char *json = fetch_device_config_json(context->endpoint);
    
    if (json) {
        console_info(&csl, "Configuration fetched successfully");
        
        // Apply configuration using renderer
        if (apply_config(json, context->dev_mode) == 0) {
            console_info(&csl, "Configuration applied successfully");
        } else {
            console_error(&csl, "Failed to apply configuration");
        }
        
        free(json);
    }
}

ConfigSyncContext *start_config_sync_service(const char *endpoint, 
                                            uint32_t initial_delay_ms,
                                            uint32_t interval_ms, 
                                            bool dev_mode) {
    ConfigSyncContext *context = (ConfigSyncContext *)malloc(sizeof(ConfigSyncContext));
    if (!context) {
        console_error(&csl, "Failed to allocate memory for config sync context");
        return NULL;
    }

    context->endpoint = endpoint;
    context->dev_mode = dev_mode;
    
    console_info(&csl, "Starting config sync service with initial delay %u ms, interval %u ms", 
                 initial_delay_ms, interval_ms);
    
    context->task_id = schedule_repeating(initial_delay_ms, interval_ms, config_sync_task, context);
    if (context->task_id == 0) {
        console_error(&csl, "Failed to schedule config sync task");
        free(context);
        return NULL;
    }

    console_debug(&csl, "Successfully scheduled config sync task with ID %u", context->task_id);
    return context;
}

void clean_config_sync_context(ConfigSyncContext *context) {
    if (context) {
        if (context->task_id != 0) {
            console_debug(&csl, "Cancelling config sync task %u", context->task_id);
            cancel_task(context->task_id);
        }
        free(context);
    }
}



