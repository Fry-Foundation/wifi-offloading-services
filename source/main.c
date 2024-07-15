#include "lib/scheduler.h"
#include "services/access.h"
#include "services/accounting.h"
#include "services/config.h"
#include "services/device_data.h"
#include "services/device_status.h"
#include "services/setup.h"
#include "services/mqtt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mosquitto.h>

int main(int argc, char *argv[]) {
    // Init
    Scheduler sch = {NULL};

    init_config(argc, argv);
    init_device_data();

    // Start services and schedule future tasks on each
    access_service(&sch);
    device_status_service(&sch);
    setup_service(&sch);
    accounting_service(&sch);

    // print_tasks(&sch);
    struct mosquitto *mosq = init_mqtt();
    run_tasks(&sch);

    clean_up_mosquitto(&mosq);
    clean_device_data_service();
    clean_access_service();

    return 0;
}
