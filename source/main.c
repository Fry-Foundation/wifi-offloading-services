#include "lib/scheduler.h"
#include "services/access.h"
#include "services/accounting.h"
#include "services/config.h"
#include "services/device_data.h"
#include "services/device_status.h"
#include "services/monitoring.h"
#include "services/mqtt-cert.h"
#include "services/mqtt.h"
#include "services/registration.h"
#include "services/access_token.h"
#include "services/setup.h"
#include <mosquitto.h>
#include <stdbool.h>
#include <unistd.h>

// @todo reschedule device registration if it fails
// @todo reschedule access token refresh if it fails
int main(int argc, char *argv[]) {
    // Init
    Scheduler *sch = init_scheduler();
    init_config(argc, argv);
    init_device_data();
    bool valid_registration = init_registration(device_data.mac, device_data.model, device_data.brand);
    if (!valid_registration) return 1;
    Registration registration = get_registration();
    AccessToken *access_token = init_access_token(&registration);
    generate_and_sign_cert();
    struct mosquitto *mosq = init_mqtt();

    // Start services and schedule future tasks on each
    access_service(sch);
    device_status_service(sch);
    setup_service(sch);
    accounting_service(sch);
    monitoring_service(sch, mosq);

    run_tasks(sch);

    // Clean up
    clean_up_mosquitto(&mosq);
    clean_scheduler(sch);
    clean_device_data_service();
    clean_access_service();

    return 0;
}
