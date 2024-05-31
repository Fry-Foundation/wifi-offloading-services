#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "utils/console.h"

Config config = {0};

void init_config(int argc, char *argv[]) {
    // Initial values
    config.dev_env = false;
    config.enabled = true;
    strcpy(config.main_api, "https://api.wayru.tech");
    strcpy(config.accounting_api, "https://wifi.api.wayru.tech");
    config.accounting_enabled = true;
    config.accounting_interval = 300;
    config.access_task_interval = 120;

    printf("hi");

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
        if (strcmp(argv[i], "--config-access-task-interval") == 0) {
            config.access_task_interval = atoi(argv[i + 1]);
            continue;
        }
        
        // Log level (note that the log level is not part of the config struct)
        if (strcmp(argv[i], "--config-console-log-level") == 0) {
            int console_log_level = atoi(argv[i + 1]);
            set_console_level(console_log_level);
            continue;
        }
    }

    // Set active paths
    if (config.dev_env == 1) {
        strcpy(config.active_path, ".");
        strcpy(config.scripts_path, "./scripts");
        strcpy(config.data_path, "./data");
    } else {
        strcpy(config.active_path, "/etc/wayru-os-services");
        strcpy(config.scripts_path, "/etc/wayru-os-services/scripts");
        strcpy(config.data_path, "/etc/wayru-os-services/data");
    }

    // Print for debug purposes
    console(CONSOLE_DEBUG, "config.dev_env: %d", config.dev_env);
    console(CONSOLE_DEBUG, "config.enabled: %d", config.enabled);
    console(CONSOLE_DEBUG, "config.main_api: %s", config.main_api);
    console(CONSOLE_DEBUG, "config.accounting_api: %s", config.accounting_api);
    console(CONSOLE_DEBUG, "config.accounting_enabled: %d", config.accounting_enabled);
    console(CONSOLE_DEBUG, "config.accounting_interval: %d", config.accounting_interval);
    console(CONSOLE_DEBUG, "config.access_task_interval: %d", config.access_task_interval);
    console(CONSOLE_DEBUG, "config.active_path: %s", config.active_path);
    console(CONSOLE_DEBUG, "config.scripts_path: %s", config.scripts_path);
    console(CONSOLE_DEBUG, "config.data_path: %s", config.data_path);

    // Fin
}

