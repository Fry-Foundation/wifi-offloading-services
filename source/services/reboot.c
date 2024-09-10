#include "reboot.h"
#include "lib/console.h"
#include "lib/scheduler.h"
#include "lib/script_runner.h"
#include "services/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void execute_reboot() {
    if (config.dev_env)
        console(CONSOLE_DEBUG, "Running reboot command...but not");
    else{
        console(CONSOLE_DEBUG, "Running reboot command");
        run_script("reboot now");
    }
}

void reboot_task(Scheduler *sch, void *task_context) {
    (void)task_context;

    if (config.reboot_enabled == 0) {
        console(CONSOLE_DEBUG, "Reboot is disabled by configuration; will not reschedule reboot task.");
        return;
    }

    console(CONSOLE_DEBUG, "Executing scheduled reboot task.");
    execute_reboot();

    //schedule_task(sch, time(NULL) + config.reboot_interval, reboot_task, "reboot", NULL);
}

void reboot_service(Scheduler *sch) {
    if (config.reboot_enabled) {
        console(CONSOLE_DEBUG, "Scheduling reboot task.");
        //schedule_task(sch, time(NULL) + config.firmware_upgrade_interval+20, reboot_task, "reboot", NULL);

        schedule_task(sch, time(NULL) + config.reboot_interval, reboot_task, "reboot", NULL);
    } else {
        console(CONSOLE_DEBUG, "Reboot service is disabled in configuration.");
    }
}