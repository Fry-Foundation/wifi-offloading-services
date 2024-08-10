#include "lib/scheduler.h"
#include "services/access.h"
#include "services/access_token.h"
#include "services/accounting.h"
#include "services/config.h"
#include "services/device-context.h"
#include "services/device_info.h"
#include "services/device_status.h"
#include "services/monitoring.h"
#include "services/mqtt-cert.h"
#include "services/mqtt.h"
#include "services/registration.h"
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
    DeviceInfo *device_info = init_device_info();
    Registration *registration = init_registration(device_info->mac, device_info->model, device_info->brand);
    AccessToken *access_token = init_access_token(registration);
    DeviceContext *device_context = init_device_context(registration, access_token);
    generate_and_sign_cert();
    struct mosquitto *mosq = init_mqtt(access_token);

    // Start services and schedule future tasks on each
    access_service(sch, device_info);
    device_status_service(sch);
    setup_service(sch);
    accounting_service(sch);
    monitoring_service(sch, mosq, registration);

    run_tasks(sch);

    // Clean up
    clean_up_mosquitto(&mosq);
    clean_scheduler(sch);
    clean_device_info(device_info);
    clean_access_service();
    clean_registration(registration);

    return 0;
}
