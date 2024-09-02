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
#include "services/firmware_upgrade.h"
#include "services/site-clients.h"
#include "services/speedtest.h"
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
    firmware_upgrade_on_boot(registration, device_info);
    get_ca_cert(access_token);
    generate_and_sign_cert(access_token);
    DeviceContext *device_context = init_device_context(registration, access_token);
    struct mosquitto *mosq = init_mqtt(registration, access_token);
    int site_clients_fifo_fd = init_site_clients_fifo();

    // Start services and schedule future tasks on each
    access_token_service(sch, access_token, registration);
    device_context_service(sch, device_context, registration, access_token);
    device_status_service(sch, device_info, registration->wayru_device_id);
    setup_service(sch, device_info, registration->wayru_device_id);
    accounting_service(sch);
    monitoring_service(sch, mosq, registration);
    firmware_upgrade_check(sch, device_info, registration);
    site_clients_service(sch, mosq, site_clients_fifo_fd, device_context->site);
    speedtest_service(sch, mosq, registration, access_token);
    commands_service(mosq, device_info, registration);

    run_tasks(sch);

    // Clean up
    clean_site_clients_fifo(site_clients_fifo_fd);
    clean_up_mosquitto(&mosq);
    clean_access_service();
    clean_device_context(device_context);
    clean_access_token(access_token);
    clean_registration(registration);
    clean_device_info(device_info);
    clean_scheduler(sch);

    return 0;
}
