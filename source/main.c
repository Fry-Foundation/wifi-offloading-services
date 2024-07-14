#include "lib/scheduler.h"
#include "services/access.h"
#include "services/accounting.h"
#include "services/config.h"
#include "services/device_data.h"
#include "services/device_status.h"
#include "services/setup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // Init
    Scheduler *sch = init_scheduler();
    init_config(argc, argv);
    init_device_data();

    // Start services and schedule future tasks on each
    access_service(sch);
    device_status_service(sch);
    setup_service(sch);
    accounting_service(sch);

    run_tasks(sch);

    clean_scheduler(sch);
    clean_device_data_service();
    clean_access_service();

    return 0;
}
