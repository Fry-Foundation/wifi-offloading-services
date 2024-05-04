#include "services/access.h"
#include "services/accounting.h"
#include "services/init.h"
#include "services/peaq_id.h"
#include "services/scheduler.h"
#include "services/setup.h"
#include "store/config.h"
#include "store/state.h"
#include "utils/console.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    init(argc, argv);
    int accounting_interval = getConfig().accounting_interval;
    int access_task_interval = getConfig().access_task_interval;

    console(CONSOLE_INFO, "running peaq_id_task");
    peaq_id_task();

    // Request access key to get backend status
    // Note that we disregard the expiration time for now,
    // but the periodic access task does check expiration
    // @TODO: This should probably be part of the access key initialization
    request_access_key(state.access_key);
    write_access_key(state.access_key);
    configure_with_access_status(state.access_status);
    state.onBoot = 0;

    Scheduler sch = {NULL, 0};

    // Schedule the access task with an interval of 2 minutes
    scheduleEvery(&sch, access_task_interval, access_task);

    // Schedule the setup task for now, and then with an interval of 1 minute
    scheduleAt(&sch, time(NULL), setupTask);
    scheduleEvery(&sch, 60, setupTask);

    // Schedule the accounting task with an interval of 1 minute
    scheduleAt(&sch, time(NULL), accounting_task);
    scheduleEvery(&sch, accounting_interval, accounting_task);

    run(&sch);

    cleanState();
    cleanConfig();

    return 0;
}
