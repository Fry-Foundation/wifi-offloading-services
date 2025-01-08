#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#define PATH_SIZE 256
#define API_SIZE 256
#define API_KEY_SIZE 1024

typedef struct {
    bool dev_env;
    bool enabled;

    char main_api[API_SIZE];

    char accounting_api[API_SIZE];

    bool accounting_enabled;
    int accounting_interval;

    int access_interval;

    int device_status_interval;

    int setup_interval;

    bool monitoring_enabled;
    int monitoring_interval;
    int monitoring_minimum_interval;
    int monitoring_maximum_interval;

    bool firmware_update_enabled;
    int firmware_update_interval;

    bool speed_test_enabled;
    int speed_test_interval;
    int speed_test_minimum_interval;
    int speed_test_maximum_interval;
    int speed_test_backhaul_attempts;
    int speed_test_latency_attempts;
    int speed_test_upload_limit;

    int device_context_interval;

    int site_clients_interval;

    char mqtt_broker_url[API_SIZE];

    bool reboot_enabled;
    int reboot_interval;

    bool use_n_sysupgrade;

    int diagnostic_interval;

    char active_path[PATH_SIZE];
    char scripts_path[PATH_SIZE];
    char data_path[PATH_SIZE];
    char temp_path[PATH_SIZE];
} Config;

extern Config config;

void init_config(int argc, char *argv[]);

#endif // CONFIG_H
