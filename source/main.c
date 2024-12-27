#include "lib/console.h"
#include "lib/scheduler.h"
#include "services/access_token.h"
#include "services/accounting.h"
#include "services/commands.h"
#include "services/config.h"
#include "services/device-context.h"
#include "services/device_info.h"
#include "services/device_status.h"
#include "services/firmware_upgrade.h"
#include "services/monitoring.h"
#include "services/mqtt-cert.h"
#include "services/mqtt.h"
#include "services/radsec_cert.h"
#include "services/reboot.h"
#include "services/registration.h"
#include "services/setup.h"
#include "services/site-clients.h"
#include "services/speedtest.h"
#include "lib/network_check.h"
#include <mosquitto.h>
#include <stdbool.h>
#include <unistd.h>
#include <lib/retry.h>

static Console csl = {
    .topic = "main",
};

int main(int argc, char *argv[]) {
    // Init
    Scheduler *sch = init_scheduler();
    init_config(argc, argv);
    DeviceInfo *device_info = init_device_info();

    bool internet_status = internet_check();
    if (!internet_status) return 1;

    bool wayru_status = wayru_check();
    if (!wayru_status) return 1;

    Registration *registration =
        init_registration(device_info->mac, device_info->model, device_info->brand, device_info->device_id);

    AccessToken *access_token = init_access_token(registration);
    if (access_token == NULL) {
        print_error(&csl, "Failed to start access token ... exiting");
        return 1;
    }

    firmware_upgrade_on_boot(registration, device_info, access_token);

    // @todo-later check if this is the appropriate CA, and download it if it's not
    bool ca_cert_result = attempt_ca_cert(access_token);
    if (!ca_cert_result) return 1;

    bool generate_and_sign_result = attempt_generate_and_sign(access_token);
    if (!generate_and_sign_result) return 1;

    bool radsec_cert_result = attempt_radsec_ca_cert(access_token);
    if (!radsec_cert_result) return 1;

    bool generate_and_sign_radsec_result = attempt_generate_and_sign_radsec(access_token, registration);
    if (!generate_and_sign_radsec_result) return 1;

    install_radsec_cert();

    DeviceContext *device_context = init_device_context(registration, access_token);
    struct mosquitto *mosq = init_mqtt(registration, access_token);
    int site_clients_fifo_fd = init_site_clients_fifo();

    // Start services and schedule future tasks on each
    access_token_service(sch, access_token, registration, mosq);
    device_context_service(sch, device_context, registration, access_token);
    device_status_service(sch, device_info, registration->wayru_device_id, access_token);
    setup_service(sch, device_info, registration->wayru_device_id, access_token);
    accounting_service(sch);
    monitoring_service(sch, mosq, registration);
    firmware_upgrade_check(sch, device_info, registration, access_token);

    site_clients_service(sch, mosq, site_clients_fifo_fd, device_context->site);
    speedtest_service(sch, mosq, registration, access_token);
    commands_service(mosq, device_info, registration, access_token);
    reboot_service(sch);

    run_tasks(sch);

    // Clean up
    clean_site_clients_fifo(site_clients_fifo_fd);
    clean_up_mosquitto(&mosq);
    clean_device_context(device_context);
    clean_access_token(access_token);
    clean_registration(registration);
    clean_device_info(device_info);
    clean_scheduler(sch);

    return 0;
}
