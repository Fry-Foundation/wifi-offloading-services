#include "services/access.h"
#include "services/config.h"
#include "services/accounting.h"
#include "services/peaq_id.h"
#include "services/scheduler.h"
#include "services/setup.h"
#include "services/device_data.h"
#include "services/config.h"
#include "store/state.h"
#include "utils/console.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    init_config(argc, argv);
    init_device_data();
    AccessKey *access_key = init_access_key();
    init_state(0, access_key);

    console(CONSOLE_INFO, "running peaq_id_task");
    peaq_id_task();

    // Request access key to get backend status
    // Note that we disregard the expiration time for now,
    // but the periodic access task does check expiration
    // @TODO: This should probably be part of the access key initialization
    request_access_key(state.access_key);
    write_access_key(state.access_key);
    configure_with_access_status(state.access_status);
    state.on_boot = 0;

    Scheduler sch = {NULL, 0};

    // Schedule the access task with an interval of 2 minutes
    scheduleEvery(&sch, config.access_task_interval, access_task);

    // Schedule the setup task for now, and then with an interval of 1 minute
    scheduleAt(&sch, time(NULL), setupTask);
    scheduleEvery(&sch, 60, setupTask);

    // Schedule the accounting task with an interval of 1 minute
    scheduleAt(&sch, time(NULL), accounting_task);
    scheduleEvery(&sch, config.accounting_interval, accounting_task);

    run(&sch);

    clean_device_data();
    clean_state();

    return 0;
}
