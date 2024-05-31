#include "lib/console.h"
#include "lib/scheduler.h"
#include "services/access.h"
#include "services/accounting.h"
#include "services/config.h"
#include "services/device_data.h"
#include "services/peaq_did.h"
#include "services/setup.h"
#include "services/state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // Init
    init_config(argc, argv);
    init_device_data();
    AccessKey *access_key = init_access_key();
    init_state(0, access_key);

    // We use this to tell the backend the device has just booted up
    // Sent on the first status request
    // @todo: should probably just initialize it with this value
    state.on_boot = 0;

    Scheduler scheduler = {NULL};

    // Schedule an access key request for now and with the configured interval in the Config struct
    Task *get_access_key = create_task(time(NULL), access_task, NULL, true,
                                       config.access_task_interval, "access task");
    schedule_task(&scheduler, get_access_key);

    // Schedule the setup task for now and with a periodic interval
    Task *setup_task_task = create_task(time(NULL), setupTask, NULL, true, 60, "setup task");
    schedule_task(&scheduler, setup_task_task);

    // Schedule the accountnig task for now and with the configured interval in the Config struct
    Task *accounting_task_task = create_task(time(NULL), accounting_task, NULL, true,
                                             config.accounting_interval, "accounting task");
    schedule_task(&scheduler, accounting_task_task);

    // Print the list of tasks
    print_tasks(&scheduler);

    run(&scheduler);

    clean_device_data();
    clean_state();

    return 0;
}
