#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <stdbool.h>
#include "core/scheduler.h"

// Function declarations
bool collector_write(const char *level, const char *topic, const char *message);
void collector_init(void);
void collector_service(Scheduler *sch, char *device_id, char *access_token, int collector_interval, const char *device_api_host);

// Cleanup function
void collector_cleanup(void);

#endif // COLLECTOR_H
