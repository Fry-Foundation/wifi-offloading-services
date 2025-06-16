#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include "core/uloop_scheduler.h"

typedef struct {
    task_id_t task_id;  // Store current task ID for cleanup
} TimeSyncTaskContext;

TimeSyncTaskContext *time_sync_service(void);
void clean_time_sync_context(TimeSyncTaskContext *context);

#endif // TIME_SYNC_H
