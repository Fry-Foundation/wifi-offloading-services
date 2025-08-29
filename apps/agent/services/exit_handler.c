#include "exit_handler.h"
#include "core/console.h"
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Console csl = {
    .topic = "exit handler",
};

typedef struct {
    cleanup_callback callback;
    void *data;
} CleanupEntry;

// Function to get signal name and description
static const char *get_signal_name_and_description(int signal_num, char *buffer, size_t buffer_size) {
    switch (signal_num) {
    case SIGINT:
        snprintf(buffer, buffer_size, "SIGINT (Interrupt from keyboard/Ctrl+C)");
        break;
    case SIGTERM:
        snprintf(buffer, buffer_size, "SIGTERM (Termination request)");
        break;
    case SIGQUIT:
        snprintf(buffer, buffer_size, "SIGQUIT (Quit from keyboard/Ctrl+\\)");
        break;
    case SIGKILL:
        snprintf(buffer, buffer_size, "SIGKILL (Kill signal - cannot be caught)");
        break;
    case SIGHUP:
        snprintf(buffer, buffer_size, "SIGHUP (Hangup detected on controlling terminal)");
        break;
    case SIGABRT:
        snprintf(buffer, buffer_size, "SIGABRT (Abort signal from abort())");
        break;
    case SIGFPE:
        snprintf(buffer, buffer_size, "SIGFPE (Floating point exception)");
        break;
    case SIGSEGV:
        snprintf(buffer, buffer_size, "SIGSEGV (Segmentation fault)");
        break;
    case SIGPIPE:
        snprintf(buffer, buffer_size, "SIGPIPE (Broken pipe)");
        break;
    case SIGALRM:
        snprintf(buffer, buffer_size, "SIGALRM (Timer alarm)");
        break;
    case SIGUSR1:
        snprintf(buffer, buffer_size, "SIGUSR1 (User-defined signal 1)");
        break;
    case SIGUSR2:
        snprintf(buffer, buffer_size, "SIGUSR2 (User-defined signal 2)");
        break;
    default:
        snprintf(buffer, buffer_size, "Signal %d (Unknown or uncommon signal)", signal_num);
        break;
    }
    return buffer;
}

#define MAX_CLEANUP_CALLBACKS 10
static CleanupEntry cleanup_entries[MAX_CLEANUP_CALLBACKS];
static int cleanup_count = 0;

static bool shutdown_requested = false;
static char shutdown_reason[256] = {0};
static pthread_mutex_t lock;

void signal_handler(int signal) {
    char signal_details[128];
    get_signal_name_and_description(signal, signal_details, sizeof(signal_details));

    console_info(&csl, "Signal received: %s. Initiating shutdown ...", signal_details);

    char cleanup_reason[256];
    snprintf(cleanup_reason, sizeof(cleanup_reason), "Signal received: %s", signal_details);
    cleanup_and_exit(0, cleanup_reason);
}

void setup_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        console_error(&csl, "could not set SIGINT handler");
        exit(1);
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        console_error(&csl, "could not set SIGTERM handler");
        exit(1);
    }
}

void register_cleanup(cleanup_callback callback, void *data) {
    if (cleanup_count < MAX_CLEANUP_CALLBACKS) {
        cleanup_entries[cleanup_count].callback = callback;
        cleanup_entries[cleanup_count].data = data;
        cleanup_count++;
    } else {
        console_error(&csl, "too many cleanup functions registered");
    }
}

void cleanup_and_exit(int exit_code, const char *reason) {
    console_info(&csl, "cleaning up ... reason: %s", reason ? reason : "not specified");
    for (int i = cleanup_count - 1; i >= 0; i--) {
        if (cleanup_entries[i].callback) {
            cleanup_entries[i].callback(cleanup_entries[i].data);
        }
    }
    console_info(&csl, "exiting with code %d", exit_code);
    exit(exit_code);
}

// This function allows the MQTT client to request the program to stop executing
void request_cleanup_and_exit(const char *reason) {
    pthread_mutex_lock(&lock);
    shutdown_requested = true;
    if (reason) {
        strncpy(shutdown_reason, reason, sizeof(shutdown_reason) - 1);
        shutdown_reason[sizeof(shutdown_reason) - 1] = '\0';
    } else {
        strncpy(shutdown_reason, "Shutdown requested", sizeof(shutdown_reason) - 1);
    }
    pthread_mutex_unlock(&lock);
}

bool is_shutdown_requested() {
    bool result;
    pthread_mutex_lock(&lock);
    result = shutdown_requested;
    pthread_mutex_unlock(&lock);
    return result;
}

const char *get_shutdown_reason() {
    const char *result;
    pthread_mutex_lock(&lock);
    result = shutdown_reason[0] ? shutdown_reason : "Shutdown requested";
    pthread_mutex_unlock(&lock);
    return result;
}
