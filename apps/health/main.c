#include "core/console.h"
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

static Console csl = {
    .topic = "main",
};

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

int main(int argc, char *argv[]) {
    bool dev_env = false;

    // Process command line arguments
    process_command_line_args(argc, argv, &dev_env);

    if (dev_env) {
        console_info(&csl, "Health service started in development mode");
    } else {
        console_info(&csl, "Health service started");
    }

    // TODO: Add health check logic here
    // For now, just keep the service running
    while (1) {
        // Simple health check loop - could be expanded to check system health
        // Sleep for a while to prevent busy waiting
        sleep(30);
    }

    return 0;
}