#ifndef SYNC_H
#define SYNC_H

#include "core/uloop_scheduler.h"
#include <stdint.h>
#include <stdbool.h>

struct ConfigSyncContext {
    task_id_t task_id;
    const char *endpoint;
    bool dev_mode;
};

typedef struct ConfigSyncContext ConfigSyncContext;

ConfigSyncContext *start_config_sync_service(const char *endpoint, 
                                            uint32_t initial_delay_ms,
                                            uint32_t interval_ms, 
                                            bool dev_mode);
void clean_config_sync_context(ConfigSyncContext *context);
char *fetch_device_config_json(const char *endpoint);

#endif /* SYNC_H */
