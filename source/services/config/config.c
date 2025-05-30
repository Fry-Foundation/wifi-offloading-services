#include "config.h"
#include "lib/console.h"
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
        // In dev mode, try multiple possible paths for the config file
        // First try relative to output directory (when run via dev.sh)
        snprintf(config_file_path, buffer_size, "%s", DEV_CONFIG_PATH_1);
        FILE *test_file = fopen(config_file_path, "r");
        if (test_file == NULL) {
            // If that fails, try relative to project root (when run directly)
            snprintf(config_file_path, buffer_size, "%s", DEV_CONFIG_PATH_2);
        } else {
            fclose(test_file);
        }
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
            set_console_level(console_log_level);
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
        print_error(&csl, "Failed to determine config file path");
        exit(1);
    }

    // Parse the UCI config file
    if (!parse_uci_config(config_file_path, &config)) {
        print_error(&csl, "Failed to parse config file, using defaults");
    }

    // Apply command line overrides
    apply_command_line_overrides(argc, argv);

    // Exit if disabled
    if (!config.enabled) {
        print_info(&csl, "Service is disabled via configuration");
        exit(0);
    }

    // Print configuration for debugging
    print_config_debug();
}

void print_config_debug(void) {
    print_debug(&csl, "config.dev_env: %d", config.dev_env);
    print_debug(&csl, "config.enabled: %d", config.enabled);
    print_debug(&csl, "config.main_api: %s", config.main_api);
    print_debug(&csl, "config.accounting_api: %s", config.accounting_api);
    print_debug(&csl, "config.devices_api: %s", config.devices_api);
    print_debug(&csl, "config.access_interval: %d", config.access_interval);
    print_debug(&csl, "config.device_status_interval: %d", config.device_status_interval);
    print_debug(&csl, "config.active_path: %s", config.active_path);
    print_debug(&csl, "config.scripts_path: %s", config.scripts_path);
    print_debug(&csl, "config.data_path: %s", config.data_path);
    print_debug(&csl, "config.temp_path: %s", config.temp_path);
    print_debug(&csl, "config.monitoring_enabled: %d", config.monitoring_enabled);
    print_debug(&csl, "config.monitoring_interval: %d", config.monitoring_interval);
    print_debug(&csl, "config.monitoring_minimum_interval: %d", config.monitoring_minimum_interval);
    print_debug(&csl, "config.monitoring_maximum_interval: %d", config.monitoring_maximum_interval);
    print_debug(&csl, "config.speed_test_enabled: %d", config.speed_test_enabled);
    print_debug(&csl, "config.speed_test_interval: %d", config.speed_test_interval);
    print_debug(&csl, "config.speed_test_latency_attempts: %d", config.speed_test_latency_attempts);
    print_debug(&csl, "config.device_context_interval: %d", config.device_context_interval);
    print_debug(&csl, "config.mqtt_broker_url: %s", config.mqtt_broker_url);
    print_debug(&csl, "config.mqtt_keepalive: %d", config.mqtt_keepalive);
    print_debug(&csl, "config.mqtt_task_interval: %d", config.mqtt_task_interval);
    print_debug(&csl, "config.reboot_enabled: %d", config.reboot_enabled);
    print_debug(&csl, "config.reboot_interval: %d", config.reboot_interval);
    print_debug(&csl, "config.firmware_update_enabled: %d", config.firmware_update_enabled);
    print_debug(&csl, "config.firmware_update_interval: %d", config.firmware_update_interval);
    print_debug(&csl, "config.use_n_sysupgrade: %d", config.use_n_sysupgrade);
    print_debug(&csl, "config.package_update_enabled: %d", config.package_update_enabled);
    print_debug(&csl, "config.package_update_interval: %d", config.package_update_interval);
    print_debug(&csl, "config.diagnostic_interval: %d", config.diagnostic_interval);
    print_debug(&csl, "config.external_connectivity_host: %s", config.external_connectivity_host);
    print_debug(&csl, "config.nds_interval: %d", config.nds_interval);
    print_debug(&csl, "config.time_sync_server: %s", config.time_sync_server);
    print_debug(&csl, "config.time_sync_interval: %d", config.time_sync_interval);
}