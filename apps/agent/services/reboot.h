#ifndef REBOOT_H
#define REBOOT_H

#include "core/uloop_scheduler.h"

typedef struct {
    task_id_t task_id;  // Store current task ID for cleanup
} RebootTaskContext;

void execute_reboot();
void reboot_task(void *task_context);
RebootTaskContext *reboot_service(void);
void clean_reboot_context(RebootTaskContext *context);

#endif // REBOOT_H
