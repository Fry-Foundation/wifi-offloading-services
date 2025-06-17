#include "reboot.h"
#include "core/console.h"
#include "core/script_runner.h"
#include "core/uloop_scheduler.h"
#include "services/config/config.h"
#include <stdlib.h>
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

void reboot_task(void *task_context) {
    (void)task_context;

    if (config.reboot_enabled == 0) {
        console_debug(&csl, "reboot is disabled by configuration; will not reschedule reboot task");
        return;
    }

    console_debug(&csl, "executing scheduled reboot task.");
    execute_reboot();

    // No manual rescheduling needed - repeating tasks auto-reschedule
}

RebootTaskContext *reboot_service(void) {
    if (!config.reboot_enabled) {
        console_debug(&csl, "reboot service is disabled in configuration");
        return NULL;
    }

    RebootTaskContext *context = (RebootTaskContext *)malloc(sizeof(RebootTaskContext));
    if (context == NULL) {
        console_error(&csl, "failed to allocate memory for reboot task context");
        return NULL;
    }

    context->task_id = 0;

    // Convert seconds to milliseconds for scheduler
    uint32_t interval_ms = config.reboot_interval * 1000;
    uint32_t initial_delay_ms = config.reboot_interval * 1000; // Start after one interval

    console_info(&csl, "Starting reboot service with interval %u ms", interval_ms);

    // Schedule repeating task
    context->task_id = schedule_repeating(initial_delay_ms, interval_ms, reboot_task, context);

    if (context->task_id == 0) {
        console_error(&csl, "failed to schedule reboot task");
        free(context);
        return NULL;
    }

    console_debug(&csl, "Successfully scheduled reboot task with ID %u", context->task_id);
    return context;
}

void clean_reboot_context(RebootTaskContext *context) {
    console_debug(&csl, "clean_reboot_context called with context: %p", context);
    if (context != NULL) {
        if (context->task_id != 0) {
            console_debug(&csl, "Cancelling reboot task %u", context->task_id);
            cancel_task(context->task_id);
        }
        console_debug(&csl, "Freeing reboot context %p", context);
        free(context);
    }
}
