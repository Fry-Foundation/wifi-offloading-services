#include "defaults.h"
#include "config.h"
#include <string.h>

void apply_config_defaults(Config *config) {
    // Reset the structure to zero first
    memset(config, 0, sizeof(Config));

    // Set default values
    config->dev_env = false;
    config->enabled = DEFAULT_ENABLED;

    strcpy(config->main_api, DEFAULT_MAIN_API);
    strcpy(config->accounting_api, DEFAULT_ACCOUNTING_API);
    strcpy(config->devices_api, DEFAULT_DEVICES_API);

    config->access_interval = DEFAULT_ACCESS_INTERVAL;
    config->device_status_interval = DEFAULT_DEVICE_STATUS_INTERVAL;

    config->monitoring_enabled = DEFAULT_MONITORING_ENABLED;
    config->monitoring_interval = DEFAULT_MONITORING_INTERVAL;
    config->monitoring_minimum_interval = DEFAULT_MONITORING_MINIMUM_INTERVAL;
    config->monitoring_maximum_interval = DEFAULT_MONITORING_MAXIMUM_INTERVAL;

    config->speed_test_enabled = DEFAULT_SPEED_TEST_ENABLED;
    config->speed_test_interval = DEFAULT_SPEED_TEST_INTERVAL;
    config->speed_test_minimum_interval = DEFAULT_SPEED_TEST_MINIMUM_INTERVAL;
    config->speed_test_maximum_interval = DEFAULT_SPEED_TEST_MAXIMUM_INTERVAL;
    config->speed_test_latency_attempts = DEFAULT_SPEED_TEST_LATENCY_ATTEMPTS;

    config->device_context_interval = DEFAULT_DEVICE_CONTEXT_INTERVAL;

    strcpy(config->mqtt_broker_url, DEFAULT_MQTT_BROKER_URL);
    config->mqtt_keepalive = DEFAULT_MQTT_KEEPALIVE;
    config->mqtt_task_interval = DEFAULT_MQTT_TASK_INTERVAL;

    config->reboot_enabled = DEFAULT_REBOOT_ENABLED;
    config->reboot_interval = DEFAULT_REBOOT_INTERVAL;

    config->firmware_update_enabled = DEFAULT_FIRMWARE_UPDATE_ENABLED;
    config->firmware_update_interval = DEFAULT_FIRMWARE_UPDATE_INTERVAL;
    config->use_n_sysupgrade = DEFAULT_USE_N_SYSUPGRADE;

    config->package_update_enabled = DEFAULT_PACKAGE_UPDATE_ENABLED;
    config->package_update_interval = DEFAULT_PACKAGE_UPDATE_INTERVAL;

    config->diagnostic_interval = DEFAULT_DIAGNOSTIC_INTERVAL;
    strcpy(config->external_connectivity_host, DEFAULT_EXTERNAL_CONNECTIVITY_HOST);

    config->nds_interval = DEFAULT_NDS_INTERVAL;

    strcpy(config->time_sync_server, DEFAULT_TIME_SYNC_SERVER);
    config->time_sync_interval = DEFAULT_TIME_SYNC_INTERVAL;

    config->collector_enabled = DEFAULT_COLLECTOR_ENABLED;
    config->collector_interval = DEFAULT_COLLECTOR_INTERVAL;
}

void set_config_paths(Config *config, bool dev_env) {
    config->dev_env = dev_env;

    if (dev_env) {
        strcpy(config->active_path, DEV_ACTIVE_PATH);
        strcpy(config->scripts_path, DEV_SCRIPTS_PATH);
        strcpy(config->data_path, DEV_DATA_PATH);
        strcpy(config->temp_path, DEV_TEMP_PATH);
    } else {
        strcpy(config->active_path, PROD_ACTIVE_PATH);
        strcpy(config->scripts_path, PROD_SCRIPTS_PATH);
        strcpy(config->data_path, PROD_DATA_PATH);
        strcpy(config->temp_path, PROD_TEMP_PATH);
    }
}
