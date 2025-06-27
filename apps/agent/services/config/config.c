#include "config.h"
#include "core/console.h"
#include "defaults.h"
#include "uci_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Console csl = {
    .topic = "config",
};

// Global config instance
Config config = {0};

/**
 * Determine the config file path based on environment and availability
 * @param dev_env Whether we're in development environment
 * @param config_file_path Buffer to store the determined path
 * @param buffer_size Size of the buffer
 * @return true if a valid config file path was determined, false otherwise
 */
static bool determine_config_file_path(bool dev_env, char *config_file_path, size_t buffer_size) {
    if (dev_env) {
        // In dev mode, use the dev config file
        snprintf(config_file_path, buffer_size, "%s", DEV_CONFIG_PATH);
        return true;
    } else {
        // In production mode, use the standard OpenWrt config path
        snprintf(config_file_path, buffer_size, "%s", PROD_CONFIG_PATH);
        return true;
    }
}

/**
 * Process command line arguments
 * @param argc Number of arguments
 * @param argv Array of arguments
 * @param dev_env Pointer to store whether dev environment was requested
 * @return true if processing was successful, false if program should exit
 */
static bool process_command_line_args(int argc, char *argv[], bool *dev_env) {
    *dev_env = false;

    // Check for --dev flag first
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dev") == 0) {
            *dev_env = true;
            break;
        }
    }

    return true;
}

/**
 * Apply command line overrides to the configuration
 * @param argc Number of arguments
 * @param argv Array of arguments
 */
static void apply_command_line_overrides(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        // Console log level can be overridden from command line
        if (strcmp(argv[i], "--config-console-log-level") == 0 && i + 1 < argc) {
            int console_log_level = atoi(argv[i + 1]);
            console_set_level(console_log_level);
            i++; // Skip the next argument as it's the value
        }
    }
}

void init_config(int argc, char *argv[]) {
    bool dev_env = false;

    // Process command line arguments
    if (!process_command_line_args(argc, argv, &dev_env)) {
        exit(1);
    }

    // Apply default configuration values
    apply_config_defaults(&config);

    // Set paths based on environment
    set_config_paths(&config, dev_env);

    // Determine config file path
    char config_file_path[PATH_SIZE];
    if (!determine_config_file_path(dev_env, config_file_path, sizeof(config_file_path))) {
        console_error(&csl, "Failed to determine config file path");
        exit(1);
    }

    // Parse the UCI config file
    if (!parse_uci_config(config_file_path, &config)) {
        console_error(&csl, "Failed to parse config file, using defaults");
    }

    // Apply command line overrides
    apply_command_line_overrides(argc, argv);

    // Exit if disabled
    if (!config.enabled) {
        console_info(&csl, "Service is disabled via configuration");
        exit(0);
    }

    // Print configuration for debugging
    print_config_debug();
}

void print_config_debug(void) {
    console_debug(&csl, "config.dev_env: %d", config.dev_env);
    console_debug(&csl, "config.enabled: %d", config.enabled);
    console_debug(&csl, "config.main_api: %s", config.main_api);
    console_debug(&csl, "config.accounting_api: %s", config.accounting_api);
    console_debug(&csl, "config.devices_api: %s", config.devices_api);
    console_debug(&csl, "config.access_interval: %d", config.access_interval);
    console_debug(&csl, "config.device_status_interval: %d", config.device_status_interval);
    console_debug(&csl, "config.active_path: %s", config.active_path);
    console_debug(&csl, "config.scripts_path: %s", config.scripts_path);
    console_debug(&csl, "config.data_path: %s", config.data_path);
    console_debug(&csl, "config.temp_path: %s", config.temp_path);
    console_debug(&csl, "config.monitoring_enabled: %d", config.monitoring_enabled);
    console_debug(&csl, "config.monitoring_interval: %d", config.monitoring_interval);
    console_debug(&csl, "config.monitoring_minimum_interval: %d", config.monitoring_minimum_interval);
    console_debug(&csl, "config.monitoring_maximum_interval: %d", config.monitoring_maximum_interval);
    console_debug(&csl, "config.speed_test_enabled: %d", config.speed_test_enabled);
    console_debug(&csl, "config.speed_test_interval: %d", config.speed_test_interval);
    console_debug(&csl, "config.speed_test_latency_attempts: %d", config.speed_test_latency_attempts);
    console_debug(&csl, "config.device_context_interval: %d", config.device_context_interval);
    console_debug(&csl, "config.mqtt_broker_url: %s", config.mqtt_broker_url);
    console_debug(&csl, "config.mqtt_keepalive: %d", config.mqtt_keepalive);
    console_debug(&csl, "config.mqtt_task_interval: %d", config.mqtt_task_interval);
    console_debug(&csl, "config.reboot_enabled: %d", config.reboot_enabled);
    console_debug(&csl, "config.reboot_interval: %d", config.reboot_interval);
    console_debug(&csl, "config.firmware_update_enabled: %d", config.firmware_update_enabled);
    console_debug(&csl, "config.firmware_update_interval: %d", config.firmware_update_interval);
    console_debug(&csl, "config.use_n_sysupgrade: %d", config.use_n_sysupgrade);
    console_debug(&csl, "config.package_update_enabled: %d", config.package_update_enabled);
    console_debug(&csl, "config.package_update_interval: %d", config.package_update_interval);
    console_debug(&csl, "config.diagnostic_interval: %d", config.diagnostic_interval);
    console_debug(&csl, "config.external_connectivity_host: %s", config.external_connectivity_host);
    console_debug(&csl, "config.nds_interval: %d", config.nds_interval);
    console_debug(&csl, "config.time_sync_server: %s", config.time_sync_server);
    console_debug(&csl, "config.time_sync_interval: %d", config.time_sync_interval);
    console_debug(&csl, "config.collector_enabled: %d", config.collector_enabled);
    console_debug(&csl, "config.collector_interval: %d", config.collector_interval);
}
