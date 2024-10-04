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
#include "services/reboot.h"
#include "services/registration.h"
#include "services/setup.h"
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

    // @todo add internet check (ping google.com), or wait ... if wait is too long ... exit(1)
    // @todo add wayru check (HTTP GET ${config.accounting_api}/health), or wait ... exit(1)

    Registration *registration =
        init_registration(device_info->mac, device_info->model, device_info->brand, device_info->device_id);

    AccessToken *access_token = init_access_token(registration);
    if (access_token == NULL) {
        console(CONSOLE_ERROR, "Failed to start access token ... exiting");
        return 1;
    }

    firmware_upgrade_on_boot(registration, device_info, access_token);

    // @todo-later check if this is the appropriate CA, and download it if it's not
    // @todo check if the CA is a valid certificate, if not ... wait and try again ... and if not ... exit(1)
    get_ca_cert(access_token);

    // @todo add a verification to check that the .key corresponds to the .crt ... and if not ... refresh .key, .csr and .crt

    // @todo handle the case where the .crt was not received from the backend
    // - handle CURL errors (HTTP request errors) or NULL response ... we currently just return and continue

    // @todo improve the certificate verification after it is received from the backend (we if the .crt is signed by the CA)
    // ------> add a verification to check that the .key corresponds to the .crt
    int attempts = 0;
    bool result = generate_and_sign_cert(access_token);
    while (!result && attempts < 3) {
        console(CONSOLE_ERROR, "Failed to generate and sign certificate ... retrying");
        result = generate_and_sign_cert(access_token);
        attempts++;
    }
    
    if (!result) {
        console(CONSOLE_ERROR, "Failed to generate and sign certificate ... exiting");
        return 1;
    }

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
