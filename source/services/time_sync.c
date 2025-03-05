#include "time_sync.h"
#include "lib/console.h"
#include "lib/script_runner.h"
#include "services/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

static Console csl = {
    .topic = "time sync",
};

void time_sync_task(Scheduler *sch, void *task_context) {
    (void)task_context;

    char command[256];
    snprintf(command, sizeof(command), "ntpdate %s", config.time_sync_server);
    char* result = run_script(command);
    print_debug(&csl, "time sync result: %s", result);
    free(result);

    // Schedule the next time sync task
    schedule_task(sch, time(NULL) + config.time_sync_interval, time_sync_task, "time_sync", NULL);
}

void time_sync_service(Scheduler *sch) {
    // Check if dev mode
    if (config.dev_env) {
        print_warn(&csl, "dev mode is enabled, skipping time sync service");
        return;
    }

    // Check if `ntpdate` is installed
    char opennds_check_command[256];
    snprintf(opennds_check_command, sizeof(opennds_check_command), "opkg list-installed | grep ntpdate");
    bool ntpdate_installed = system(opennds_check_command) == 0;
    if (!ntpdate_installed) {
        print_warn(&csl, "ntpdate is not installed, skipping time sync service");
        return;
    }

    // Check if `ntpdate` is enabled
    char check_enabled_command[256];
    snprintf(check_enabled_command, sizeof(check_enabled_command), "service ntpdate status | grep enabled");
    bool ntpdate_enabled = system(check_enabled_command) == 0;
    if (!ntpdate_enabled) {
        print_warn(&csl, "ntpdate is not enabled, skipping time sync service");
        return;
    }

    // Schedule the time sync task
    schedule_task(sch, time(NULL) + config.time_sync_interval, time_sync_task, "time_sync", NULL);
}
