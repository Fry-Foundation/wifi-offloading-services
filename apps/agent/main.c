#include "core/console.h"
#include "core/scheduler.h"
#include "services/access_token.h"
#include "services/commands.h"
#include "services/config/config.h"
#include "services/device-context.h"
#include "services/device_info.h"
#include "services/device_status.h"
#include "services/diagnostic/diagnostic.h"
#include "services/exit_handler.h"
#include "services/firmware_upgrade.h"
#include "services/monitoring.h"
#include "services/mqtt/cert.h"
#include "services/mqtt/mqtt.h"
#include "services/nds.h"
#include "services/package_update.h"
#include "services/radsec_cert.h"
#include "services/reboot.h"
#include "services/registration.h"
#include "services/site-clients.h"
#include "services/speedtest.h"
#include "services/time_sync.h"
#include "services/collector.h"

static Console csl = {
    .topic = "main",
};

int main(int argc, char *argv[]) {
    console_info(&csl, "starting wayru-os-services");

    // Signal handlers
    setup_signal_handlers();

    // Config
    init_config(argc, argv);

    // Collector
    collector_init();

    // DeviceInfo
    DeviceInfo *device_info = init_device_info();
    register_cleanup((cleanup_callback)clean_device_info, device_info);

    // Diagnostic Init - runs DNS, internet, and Wayru reachability tests
    bool diagnostic_status = init_diagnostic_service(device_info);
    if (!diagnostic_status) {
        update_led_status(false, "Diagnostic tests failed");
        cleanup_and_exit(1, "Diagnostic tests failed");
    }

    // Registration
    Registration *registration =
        init_registration(device_info->mac, device_info->model, device_info->brand, device_info->device_id);
    register_cleanup((cleanup_callback)clean_registration, registration);

    // Access token
    AccessToken *access_token = init_access_token(registration);
    if (access_token == NULL) {
        console_error(&csl, "Failed to start access token ... exiting");
        cleanup_and_exit(1, "Failed to initialize access token");
    }
    register_cleanup((cleanup_callback)clean_access_token, access_token);

    // Package update complete signal
    check_package_update_completion(registration, device_info, access_token);

    // Firmware upgrade complete signal
    firmware_upgrade_on_boot(registration, device_info, access_token);

    // Certificate checks
    bool ca_cert_result = attempt_ca_cert(access_token);
    if (!ca_cert_result) cleanup_and_exit(1, "Failed to obtain CA certificate");

    bool generate_and_sign_result = attempt_generate_and_sign(access_token);
    if (!generate_and_sign_result) cleanup_and_exit(1, "Failed to generate and sign certificate");

    bool radsec_cert_result = attempt_radsec_ca_cert(access_token);
    if (!radsec_cert_result) cleanup_and_exit(1, "Failed to obtain RADSEC CA certificate");

    bool generate_and_sign_radsec_result = attempt_generate_and_sign_radsec(access_token, registration);
    if (!generate_and_sign_radsec_result) cleanup_and_exit(1, "Failed to generate and sign RADSEC certificate");

    install_radsec_cert();

    // Device context (site and config)
    DeviceContext *device_context = init_device_context(registration, access_token);
    register_cleanup((cleanup_callback)clean_device_context, device_context);

    // MQTT
    MqttConfig mqtt_config = {
        .client_id = registration->wayru_device_id,
        .username = access_token->token,
        .password = "any",
        .broker_url = config.mqtt_broker_url,
        .data_path = config.data_path,
        .keepalive = config.mqtt_keepalive,
        .task_interval = config.mqtt_task_interval,
    };
    MqttClient mqtt_client = {
        .mosq = init_mqtt(&mqtt_config),
        .config = mqtt_config,
    };
    register_cleanup((cleanup_callback)cleanup_mqtt, &mqtt_client.mosq);

    // NDS
    NdsClient *nds_client = init_nds_client();
    register_cleanup((cleanup_callback)clean_nds_fifo, &nds_client->fifo_fd);

    // Site clients
    init_site_clients(mqtt_client.mosq, device_context->site, nds_client);

    // Scheduler
    Scheduler *sch = init_scheduler();
    register_cleanup((cleanup_callback)clean_scheduler, sch);

    // Create MQTT access token refresh callback
    AccessTokenCallbacks token_callbacks = create_mqtt_token_callbacks(&mqtt_client);

    // Schedule service tasks
    time_sync_service(sch);
    access_token_service(sch, access_token, registration, &token_callbacks);
    mqtt_service(sch, mqtt_client.mosq, &mqtt_client.config);
    device_context_service(sch, device_context, registration, access_token);
    device_status_service(sch, device_info, registration->wayru_device_id, access_token);
    nds_service(sch, mqtt_client.mosq, device_context->site, nds_client, device_info);
    monitoring_service(sch, mqtt_client.mosq, registration);
    firmware_upgrade_check(sch, device_info, registration, access_token);
    package_update_service(sch, device_info, registration, access_token);
    start_diagnostic_service(sch, access_token);
    speedtest_service(sch, mqtt_client.mosq, registration, access_token);
    commands_service(mqtt_client.mosq, device_info, registration, access_token);
    reboot_service(sch);
    collector_service(sch, registration->wayru_device_id, access_token->token, config.collector_interval, config.devices_api);

    run_tasks(sch);

    return 0;
}
