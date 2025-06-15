#ifndef REBOOT_H
#define REBOOT_H

#include "core/scheduler.h"

void execute_reboot();
void reboot_task(Scheduler *sch, void *task_context);
void reboot_service(Scheduler *sch);

#endif // REBOOT_H
