#include "lib/scheduler.h"
#include "services/access.h"
#include "services/accounting.h"
#include "services/config.h"
#include "services/device_data.h"
#include "services/device_status.h"
#include "services/setup.h"
#include "services/mqtt-cert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // Init
    Scheduler sch = {NULL};

    init_config(argc, argv);
    init_device_data();

    generate_and_sign_cert();

    // Start services and schedule future tasks on each
    access_service(&sch);
    device_status_service(&sch);
    setup_service(&sch);
    accounting_service(&sch);

    // print_tasks(&sch);

    run_tasks(&sch);

    clean_device_data_service();
    clean_access_service();

    return 0;
}
