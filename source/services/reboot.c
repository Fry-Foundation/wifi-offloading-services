#include "reboot.h"
#include "lib/console.h"
#include "lib/scheduler.h"
#include "lib/script_runner.h"
#include "services/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static Console csl = {
    .topic = "reboot",
};
void execute_reboot() {
    if (config.dev_env)
        print_debug(&csl, "Running reboot command ... but not rebooting because we are on dev mode");
    else {
        print_debug(&csl, "Running reboot command");
        run_script("reboot now");
    }
}

void reboot_task(Scheduler *sch, void *task_context) {
    (void)task_context;

    if (config.reboot_enabled == 0) {
        print_debug(&csl, "reboot is disabled by configuration; will not reschedule reboot task");
        return;
    }

    print_debug(&csl, "executing scheduled reboot task.");
    execute_reboot();

    // schedule_task(sch, time(NULL) + config.reboot_interval, reboot_task, "reboot", NULL);
}

void reboot_service(Scheduler *sch) {
    if (config.reboot_enabled) {
        print_debug(&csl, "scheduling reboot task");

        schedule_task(sch, time(NULL) + config.reboot_interval, reboot_task, "reboot", NULL);
    } else {
        print_debug(&csl, "reboot service is disabled in configuration");
    }
}
