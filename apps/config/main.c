#include "core/console.h"
#include "core/uloop_scheduler.h"
#include "sync/sync.h"
#include "config.h"
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static Console csl = {
    .topic = "config-main",
};

static ConfigSyncContext *sync_context = NULL;
static bool dev_env = false;

static void cleanup(void) {
    if (sync_context) {
        clean_config_sync_context(sync_context);
        sync_context = NULL;
    }
}

static bool process_command_line_args(int argc, char *argv[]) {
    dev_env = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dev") == 0) {
            dev_env = true;
        }
    }
    return true;
}

int main(int argc, char *argv[]) {
    console_set_syslog_facility(CONSOLE_FACILITY_DAEMON);
    console_set_channels(CONSOLE_CHANNEL_SYSLOG | CONSOLE_CHANNEL_STDIO);
    console_set_identity("wayru-config");

    if (!process_command_line_args(argc, argv)) {
        return 0;
    }

    console_info(&csl, "Starting wayru-config service");

    const remote_config_t *config = config_get_current();
    if (!config) {
        console_error(&csl, "Failed to load configuration");
        return 1;
    }

    if (!config_is_enabled()) {
        console_info(&csl, "Configuration service is disabled");
        return 0;
    }

    if (config->config_loaded) {
        console_set_level(config->console_log_level);
        console_info(&csl, "Console log level set to %d", config->console_log_level);
    }

    if (dev_env) {
        console_info(&csl, "wayru-config started in DEVELOPMENT mode");
    } else {
        console_info(&csl, "wayru-config service started");
    }

    scheduler_init();
    console_info(&csl, "uloop scheduler initialized");

    const char *endpoint = config_get_config_endpoint();
    if (!endpoint) {
        console_error(&csl, "No config endpoint configured");
        cleanup();
        return 1;
    }
    
    console_info(&csl, "Using config endpoint: %s", endpoint);

    uint32_t config_interval_ms = config_get_config_interval_ms();
    uint32_t initial_delay = dev_env ? 5000 : 5000;  
    uint32_t interval = config_interval_ms; 

    console_info(&csl, "Using config interval: %u ms (%u seconds)", 
                 config_interval_ms, config_interval_ms / 1000);

    sync_context = start_config_sync_service(endpoint, initial_delay, interval, dev_env);
    if (!sync_context) {
        console_error(&csl, "Failed to start config sync service");
        cleanup();
        return 1;
    }

    console_info(&csl, "Config sync service started successfully");
    console_info(&csl, "Starting event loop");
    console_info(&csl, "Services scheduled, starting scheduler main loop");
    
    int scheduler_result = scheduler_run();
    
    console_info(&csl, "Scheduler main loop ended with result: %d", scheduler_result);
    console_info(&csl, "Shutting down config service...");

    cleanup();
    console_info(&csl, "Config service stopped");
    return 0;
}