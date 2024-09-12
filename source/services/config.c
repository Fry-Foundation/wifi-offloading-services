#include "config.h"
#include "lib/console.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Config config = {0};

void init_config(int argc, char *argv[]) {
    // Initial values
    config.dev_env = false;
    config.enabled = true;
    strcpy(config.main_api, "https://api.wayru.tech");
    strcpy(config.accounting_api, "https://wifi.api.wayru.tech");
    config.accounting_enabled = true;
    config.accounting_interval = 300;
    config.access_interval = 10800;
    config.device_status_interval = 120;
    config.setup_interval = 120;
    config.monitoring_enabled = true;
    config.monitoring_interval = 900;
    config.speed_test_enabled = true;
    config.speed_test_interval = 10800;
    config.speed_test_backhaul_attempts = 3;
    config.speed_test_latency_attempts = 4;
    config.speed_test_file_size = 0.3;
    config.device_context_interval = 900;
    config.site_clients_interval = 60;
    strcpy(config.mqtt_broker_url, "mqtt.wayru.tech");
    config.reboot_enabled = true;
    config.reboot_interval= 88200;
    config.firmware_update_enabled = true;
    config.firmware_update_interval = 86400;

    // Loop  through available daemon config parameters
    for (int i = 1; i < argc; i++) {
        // Environment (dev or prod)
        if (strcmp(argv[i], "--dev") == 0) {
            config.dev_env = true;
            continue;
        }

        // Daemon enabled flag
        if (strcmp(argv[i], "--config-enabled") == 0) {
            int enabled = atoi(argv[i + 1]);

            if (enabled == 0) {
                exit(0);
            } else {
                config.enabled = true;
            }

            continue;
        }

        // Main api
        if (strcmp(argv[i], "--config-main-api") == 0) {
            snprintf(config.main_api, sizeof(config.main_api), "%s", argv[i + 1]);
            continue;
        }

        // Accounting api
        if (strcmp(argv[i], "--config-accounting-api") == 0) {
            snprintf(config.accounting_api, sizeof(config.accounting_api), "%s", argv[i + 1]);
            continue;
        }

        // Accounting flag
        if (strcmp(argv[i], "--config-accounting-enabled") == 0) {
            int accounting_enabled = atoi(argv[i + 1]);
            config.accounting_enabled = (accounting_enabled == 1) ? true : false;
            continue;
        }

        // Accounting interval
        if (strcmp(argv[i], "--config-accounting-interval") == 0) {
            config.accounting_interval = atoi(argv[i + 1]);
            continue;
        }

        // Access interval
        if (strcmp(argv[i], "--config-access-interval") == 0) {
            config.access_interval = atoi(argv[i + 1]);
            continue;
        }

        // Device status interval
        if (strcmp(argv[i], "--config-device-status-interval") == 0) {
            config.device_status_interval = atoi(argv[i + 1]);
            continue;
        }

        // Setup interval
        if (strcmp(argv[i], "--config-setup-interval") == 0) {
            config.setup_interval = atoi(argv[i + 1]);
            continue;
        }

        // Log level (note that the log level is not part of the config struct)
        if (strcmp(argv[i], "--config-console-log-level") == 0) {
            int console_log_level = atoi(argv[i + 1]);
            set_console_level(console_log_level);
            continue;
        }

        // Monitoring flag
        if (strcmp(argv[i], "--config-monitoring-enabled") == 0) {
            int monitoring_enabled = atoi(argv[i + 1]);
            config.monitoring_enabled = (monitoring_enabled == 1) ? true : false;
            continue;
        }

        // Monitoring interval
        if (strcmp(argv[i], "--config-monitoring-interval") == 0) {
            config.monitoring_interval = atoi(argv[i + 1]);
            continue;
        }

        // Speed test flag
        if (strcmp(argv[i], "--config-speed-test-enabled") == 0) {
            int speed_test_enabled = atoi(argv[i + 1]);
            config.speed_test_enabled = (speed_test_enabled == 1) ? true : false;
            continue;
        }

        // Speed test interval
        if (strcmp(argv[i], "--config-speed-test-interval") == 0) {
            config.speed_test_interval = atoi(argv[i + 1]);
            continue;
        }

        // Speed test backhaul attempts
        if (strcmp(argv[i], "--config-speed-test-backhaul-attempts") == 0) {
            config.speed_test_backhaul_attempts = atoi(argv[i + 1]);
            continue;
        }

        // Speed test latency attempts
        if (strcmp(argv[i], "--config-speed-test-latency-attempts") == 0) {
            config.speed_test_latency_attempts = atoi(argv[i + 1]);
            continue;
        }

        // Speed test file size
        if (strcmp(argv[i], "--config-speed-test-file-size") == 0) {
            config.speed_test_file_size = atof(argv[i + 1]);
            continue;
        }

        // Device context interval
        if (strcmp(argv[i], "--config-device-context-interval") == 0) {
            config.device_context_interval = atoi(argv[i + 1]);
            continue;
        }

        // Site clients interval
        if (strcmp(argv[i], "--config-site-clients-interval") == 0) {
            config.site_clients_interval = atoi(argv[i + 1]);
            continue;
        }

        //Broker url
        if (strcmp(argv[i], "--config-mqtt-broker-url") == 0) {
            snprintf(config.mqtt_broker_url, sizeof(config.mqtt_broker_url), "%s", argv[i + 1]);
            continue;
        }

         // Reboot flag
        if (strcmp(argv[i], "--config-reboot-enabled") == 0) {
            int reboot_enabled = atoi(argv[i + 1]);
            config.reboot_enabled = (reboot_enabled == 1) ? true : false;
            continue;
        }

        // Reboot interval
        if (strcmp(argv[i], "--config-reboot-interval") == 0) {
            config.reboot_interval = atoi(argv[i + 1]);
            continue;
        }

        // Firmware update flag
        if (strcmp(argv[i], "--config-firmware-update-enabled") == 0) {
            int firmware_update_enabled = atoi(argv[i + 1]);
            config.firmware_update_enabled = (firmware_update_enabled == 1) ? true : false;
            continue;
        }

        // Firmware upgrade interval
        if (strcmp(argv[i], "--config-firmware-upgrade-interval") == 0) {
            config.firmware_update_interval = atoi(argv[i + 1]);
            continue;
        }
        
    }

    // Set active paths
    if (config.dev_env == 1) {
        strcpy(config.active_path, ".");
        strcpy(config.scripts_path, "./scripts");
        strcpy(config.data_path, "./data");
        strcpy(config.temp_path, "./tmp");
    } else {
        strcpy(config.active_path, "/etc/wayru-os-services");
        strcpy(config.scripts_path, "/etc/wayru-os-services/scripts");
        strcpy(config.data_path, "/etc/wayru-os-services/data");
        strcpy(config.temp_path, "/tmp");
    }

    // Print for debug purposes
    console(CONSOLE_DEBUG, "config.dev_env: %d", config.dev_env);
    console(CONSOLE_DEBUG, "config.enabled: %d", config.enabled);
    console(CONSOLE_DEBUG, "config.main_api: %s", config.main_api);
    console(CONSOLE_DEBUG, "config.accounting_api: %s", config.accounting_api);
    console(CONSOLE_DEBUG, "config.accounting_enabled: %d", config.accounting_enabled);
    console(CONSOLE_DEBUG, "config.accounting_interval: %d", config.accounting_interval);
    console(CONSOLE_DEBUG, "config.access_interval: %d", config.access_interval);
    console(CONSOLE_DEBUG, "config.device_status_interval: %d", config.device_status_interval);
    console(CONSOLE_DEBUG, "config.setup_interval: %d", config.setup_interval);
    console(CONSOLE_DEBUG, "config.active_path: %s", config.active_path);
    console(CONSOLE_DEBUG, "config.scripts_path: %s", config.scripts_path);
    console(CONSOLE_DEBUG, "config.data_path: %s", config.data_path);
    console(CONSOLE_DEBUG, "config.temp_path: %s", config.temp_path);
    console(CONSOLE_DEBUG, "config.monitoring_enabled: %d", config.monitoring_enabled);
    console(CONSOLE_DEBUG, "config.monitoring_interval: %d", config.monitoring_interval);
    console(CONSOLE_DEBUG, "config.speed_test_enabled: %d", config.speed_test_enabled);
    console(CONSOLE_DEBUG, "config.speed_test_interval: %d", config.speed_test_interval);
    console(CONSOLE_DEBUG, "config.speed_test_backhaul_attempts: %d", config.speed_test_backhaul_attempts);
    console(CONSOLE_DEBUG, "config.speed_test_latency_attempts: %d", config.speed_test_latency_attempts);
    console(CONSOLE_DEBUG, "config.speed_test_file_size: %f", config.speed_test_file_size);
    console(CONSOLE_DEBUG, "config.device_context_interval: %d", config.device_context_interval);
    console(CONSOLE_DEBUG, "config.mqtt_broker_url: %s", config.mqtt_broker_url);
    console(CONSOLE_DEBUG, "config.reboot_enabled: %d", config.reboot_enabled);
    console(CONSOLE_DEBUG, "config.reboot_interval: %d", config.reboot_interval);
    console(CONSOLE_DEBUG, "config.firmware_update_enabled: %d", config.firmware_update_enabled);
    console(CONSOLE_DEBUG, "config.firmware_update_interval: %d", config.firmware_update_interval);
}
