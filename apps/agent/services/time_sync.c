#include "time_sync.h"
#include "core/console.h"
#include "core/script_runner.h"
#include "core/uloop_scheduler.h"
#include "services/config/config.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static Console csl = {
    .topic = "time sync",
};

void time_sync_task(void *task_context) {
    (void)task_context; // Not used for this simple task

    console_debug(&csl, "Executing time sync task");

    char command[256];
    snprintf(command, sizeof(command), "ntpdate %s", config.time_sync_server);
    char *result = run_script(command);
    console_debug(&csl, "time sync result: %s", result);
    free(result);

    // No manual rescheduling needed - repeating tasks auto-reschedule
}

TimeSyncTaskContext *time_sync_service(void) {
    // Check if dev mode
    if (config.dev_env) {
        console_warn(&csl, "dev mode is enabled, skipping time sync service");
        return NULL;
    }

    // Check if `ntpdate` is installed
    char opennds_check_command[256];
    snprintf(opennds_check_command, sizeof(opennds_check_command), "opkg list-installed | grep ntpdate");
    bool ntpdate_installed = system(opennds_check_command) == 0;
    if (!ntpdate_installed) {
        console_warn(&csl, "ntpdate is not installed, skipping time sync service");
        return NULL;
    }

    // Check if `ntpdate` is enabled
    char check_enabled_command[256];
    snprintf(check_enabled_command, sizeof(check_enabled_command), "service ntpdate status | grep enabled");
    bool ntpdate_enabled = system(check_enabled_command) == 0;
    if (!ntpdate_enabled) {
        console_warn(&csl, "ntpdate is not enabled, skipping time sync service");
        return NULL;
    }

    // Allocate context
    TimeSyncTaskContext *context = (TimeSyncTaskContext *)malloc(sizeof(TimeSyncTaskContext));
    if (context == NULL) {
        console_error(&csl, "failed to allocate memory for time sync task context");
        return NULL;
    }

    // Convert seconds to milliseconds for scheduler
    uint32_t interval_ms = config.time_sync_interval * 1000;
    uint32_t initial_delay_ms = config.time_sync_interval * 1000; // Start after one interval

    console_info(&csl, "Starting time sync service with interval %u ms", interval_ms);

    // Schedule repeating task
    context->task_id = schedule_repeating(initial_delay_ms, interval_ms, time_sync_task, context);

    if (context->task_id == 0) {
        console_error(&csl, "failed to schedule time sync task");
        free(context);
        return NULL;
    }

    console_debug(&csl, "Successfully scheduled time sync task with ID %u", context->task_id);
    return context;
}

void clean_time_sync_context(TimeSyncTaskContext *context) {
    console_debug(&csl, "clean_time_sync_context called with context: %p", context);
    if (context != NULL) {
        if (context->task_id != 0) {
            console_debug(&csl, "Cancelling time sync task %u", context->task_id);
            cancel_task(context->task_id);
        }
        console_debug(&csl, "Freeing time sync context %p", context);
        free(context);
    }
}
