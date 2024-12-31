#include "exit_handler.h"
#include "lib/console.h"
#include <stdlib.h>
#include <signal.h>

static Console csl = {
    .topic = "exit handler",
};

typedef struct {
    cleanup_callback callback;
    void *data;
} CleanupEntry;

#define MAX_CLEANUP_CALLBACKS 10
static CleanupEntry cleanup_entries[MAX_CLEANUP_CALLBACKS];
static int cleanup_count = 0;

void signal_handler(int signal) {
    print_info(&csl, "Signal %d received. Initiating shutdown ...", signal);
    cleanup_and_exit(0);
}

void setup_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        print_error(&csl, "could not set SIGINT handler");
        exit(1);
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        print_error(&csl, "could not set SIGTERM handler");
        exit(1);
    }
}

void register_cleanup(cleanup_callback callback, void *data) {
    if (cleanup_count < MAX_CLEANUP_CALLBACKS) {
        cleanup_entries[cleanup_count].callback = callback;
        cleanup_entries[cleanup_count].data = data;
        cleanup_count++;
    } else {
        print_error(&csl, "too many cleanup functions registered");
    }
}

void cleanup_and_exit(int exit_code) {
    for (int i = cleanup_count - 1; i >= 0; i--) {
        if (cleanup_entries[i].callback) {
            cleanup_entries[i].callback(cleanup_entries[i].data);
        }
    }
    print_info(&csl, "exiting with code %d", exit_code);
    exit(exit_code);
}
