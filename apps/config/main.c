#include "core/console.h"
#include "core/uloop_scheduler.h"
#include "sync/sync.h"
#include "config.h"
#include <string.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static Console csl = {
    .topic = "config-main",
};
static ConfigSyncContext *sync_context = NULL;

/**
 * Process command line arguments
 * @param argc Number of arguments
 * @param argv Array of arguments
 * @param dev_env Pointer to store whether dev environment was requested
 * @return true if processing was successful, false if program should exit
 */
static bool process_command_line_args(int argc, char *argv[], bool *dev_env) {
    *dev_env = false;

    // Check for --dev flag
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dev") == 0) {
            *dev_env = true;
            break;
        }
    }

    return true;
}

static void cleanup(void) {
    if (sync_context) {
        clean_config_sync_context(sync_context);
    }
    scheduler_shutdown();
}

int main(int argc, char *argv[]) {
    bool dev_env = false;
    if (!process_command_line_args(argc, argv, &dev_env)) {
        return 1;
    }
    
    const remote_config_t *config = config_get_current();
    
    int log_level = config_get_console_log_level();
    console_set_log_level(log_level);  
    
    console_info(&csl, "Starting wayru-config service (log level: %d)", log_level);
    
    // Initialize scheduler 
    scheduler_init();

    if (dev_env) {
        console_info(&csl, "wayru-config started in development mode");
    } else {
        console_info(&csl, "wayru-config service started");
    }

    const remote_config_t *cfg = config_get_current();
    if (!cfg || !cfg->config_loaded) {
        console_warn(&csl, "No config file loaded, using default configuration");
    }

    if (!config_is_enabled()) {
        console_warn(&csl, "Configuration disabled, exiting");
        cleanup();
        return 0;
    }

    const char *endpoint = config_get_config_endpoint();
    if (!endpoint) {
        console_error(&csl, "No config endpoint configured");
        cleanup();
        return 1;
    }
    
    console_info(&csl, "Using config endpoint: %s", endpoint);

    uint32_t initial_delay = dev_env ? 10000 : 10000;   // Dev: 10s, Prod: 10s
    uint32_t interval = dev_env ? 60000 : 900000;      // Dev: 60s, Prod: 15 min

    // Start config sync service
    sync_context = start_config_sync_service(endpoint, initial_delay, interval, dev_env);
    if (!sync_context) {
        console_error(&csl, "Failed to start config sync service");
        cleanup();
        return 1;
    }

    console_info(&csl, "All services started, entering main loop");
    
    scheduler_run();
    
    cleanup();
    return 0;
}