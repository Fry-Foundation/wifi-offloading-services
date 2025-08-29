#ifndef SYNC_H
#define SYNC_H

#include "core/uloop_scheduler.h"
#include "ubus.h"
#include "token_manager.h"  
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

struct ConfigSyncContext {
    task_id_t task_id;
    const char *endpoint;
    bool dev_mode;
    uint32_t current_interval_ms; 

    // Token management fields
    char access_token[256];
    time_t token_expiry;
    bool token_initialized;
};

typedef struct ConfigSyncContext ConfigSyncContext;

// Core sync functions
ConfigSyncContext *start_config_sync_service(const char *endpoint, 
                                            uint32_t initial_delay_ms,
                                            uint32_t interval_ms, 
                                            bool dev_mode);
void clean_config_sync_context(ConfigSyncContext *context);
char *fetch_device_config_json(const char *endpoint, ConfigSyncContext *context);
int reload_config_sync_service(ConfigSyncContext *context);


#endif /* SYNC_H */
