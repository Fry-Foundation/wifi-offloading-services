#include "lib/console.h"
#include "lib/scheduler.h"
#include "services/access.h"
#include "services/accounting.h"
#include "services/config.h"
#include "services/device_data.h"
#include "services/device_status.h"
#include "services/setup.h"
#include "services/state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // Init
    Scheduler sch = {NULL};

    init_config(argc, argv);
    init_device_data();
    init_state();

    // Init service and schedule future tasks on each
    init_access_service(&sch);
    init_device_status_service(&sch);
    init_setup_service(&sch);
    init_accounting_service(&sch);

    // Print the list of tasks
    print_tasks(&sch);

    run(&sch);

    clean_device_data();
    clean_access_service();

    return 0;
}
