#ifndef CONFIG_DEFAULTS_H
#define CONFIG_DEFAULTS_H

#include <stdbool.h>
#include "config.h"

// Default configuration values
#define DEFAULT_ENABLED true
#define DEFAULT_MAIN_API "https://api.prod-2.wayru.tech"
#define DEFAULT_ACCOUNTING_API "https://wifi.api.wayru.tech"
#define DEFAULT_DEVICES_API "https://devices.wayru.tech"
#define DEFAULT_ACCESS_INTERVAL 10800
#define DEFAULT_DEVICE_STATUS_INTERVAL 120

#define DEFAULT_MONITORING_ENABLED true
#define DEFAULT_MONITORING_INTERVAL 900
#define DEFAULT_MONITORING_MINIMUM_INTERVAL 300
#define DEFAULT_MONITORING_MAXIMUM_INTERVAL 900

#define DEFAULT_SPEED_TEST_ENABLED true
#define DEFAULT_SPEED_TEST_INTERVAL 10800
#define DEFAULT_SPEED_TEST_MINIMUM_INTERVAL 10800
#define DEFAULT_SPEED_TEST_MAXIMUM_INTERVAL 21600
#define DEFAULT_SPEED_TEST_LATENCY_ATTEMPTS 4

#define DEFAULT_DEVICE_CONTEXT_INTERVAL 900

#define DEFAULT_MQTT_BROKER_URL "mqtt.wayru.tech"
#define DEFAULT_MQTT_KEEPALIVE 60
#define DEFAULT_MQTT_TASK_INTERVAL 30

#define DEFAULT_REBOOT_ENABLED true
#define DEFAULT_REBOOT_INTERVAL 88200

#define DEFAULT_FIRMWARE_UPDATE_ENABLED true
#define DEFAULT_FIRMWARE_UPDATE_INTERVAL 86400
#define DEFAULT_USE_N_SYSUPGRADE false

#define DEFAULT_PACKAGE_UPDATE_ENABLED true
#define DEFAULT_PACKAGE_UPDATE_INTERVAL 20000

#define DEFAULT_DIAGNOSTIC_INTERVAL 120
#define DEFAULT_EXTERNAL_CONNECTIVITY_HOST "google.com"

#define DEFAULT_NDS_INTERVAL 60

#define DEFAULT_TIME_SYNC_SERVER "ptbtime1.ptb.de"
#define DEFAULT_TIME_SYNC_INTERVAL 3600

// Development environment paths
#define DEV_ACTIVE_PATH "."
#define DEV_SCRIPTS_PATH "./scripts"
#define DEV_DATA_PATH "./data"
#define DEV_TEMP_PATH "./tmp"

// Production environment paths
#define PROD_ACTIVE_PATH "/etc/wayru-os-services"
#define PROD_SCRIPTS_PATH "/etc/wayru-os-services/scripts"
#define PROD_DATA_PATH "/etc/wayru-os-services/data"
#define PROD_TEMP_PATH "/tmp"

// Config file paths
#define DEV_CONFIG_PATH_1 "../source/scripts/openwrt/wayru-os-services-dev.config"
#define DEV_CONFIG_PATH_2 "source/scripts/openwrt/wayru-os-services-dev.config"
#define PROD_CONFIG_PATH "/etc/config/wayru-os-services"

/**
 * Apply default values to the config structure
 * @param config Pointer to the config structure to initialize
 */
void apply_config_defaults(Config* config);

/**
 * Set development or production paths in the config structure
 * @param config Pointer to the config structure
 * @param dev_env Whether to use development environment paths
 */
void set_config_paths(Config* config, bool dev_env);

#endif // CONFIG_DEFAULTS_H 