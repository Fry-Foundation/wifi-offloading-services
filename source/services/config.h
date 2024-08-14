#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#define PATH_SIZE 256
#define API_SIZE 256

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

    bool speed_test_enabled;
    int speed_test_interval;
    int speed_test_backhaul_attempts;
    int speed_test_latency_attempts;

    int device_context_interval;

    char active_path[PATH_SIZE];
    char scripts_path[PATH_SIZE];
    char data_path[PATH_SIZE];
} Config;

extern Config config;

void init_config(int argc, char *argv[]);

#endif // CONFIG_H
