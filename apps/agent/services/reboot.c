#include "reboot.h"
#include "core/console.h"
#include "core/scheduler.h"
#include "core/script_runner.h"
#include "services/config/config.h"
#include <unistd.h>

static Console csl = {
    .topic = "reboot",
};
void execute_reboot() {
    if (config.dev_env)
        console_debug(&csl, "Running reboot command ... but not rebooting because we are on dev mode");
    else {
        console_debug(&csl, "Running reboot command");
        run_script("reboot now");
    }
}

void reboot_task(Scheduler *sch, void *task_context) {
    (void)task_context;

    if (config.reboot_enabled == 0) {
        console_debug(&csl, "reboot is disabled by configuration; will not reschedule reboot task");
        return;
    }

    console_debug(&csl, "executing scheduled reboot task.");
    execute_reboot();

    // schedule_task(sch, time(NULL) + config.reboot_interval, reboot_task, "reboot", NULL);
}

void reboot_service(Scheduler *sch) {
    if (config.reboot_enabled) {
        console_debug(&csl, "scheduling reboot task");

        schedule_task(sch, time(NULL) + config.reboot_interval, reboot_task, "reboot", NULL);
    } else {
        console_debug(&csl, "reboot service is disabled in configuration");
    }
}
