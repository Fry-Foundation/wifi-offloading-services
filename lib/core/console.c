// console.c
#include "console.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Global variables
ConsoleLevel console_level = CONSOLE_LEVEL_INFO;
int console_channels = CONSOLE_CHANNEL_STDIO;
int console_syslog_facility = LOG_DAEMON;
const char *console_identity = NULL;
static int console_initialized = 0;

// Get default process name for logging
static const char *console_get_default_ident(void) {
    FILE *self;
    static char line[64];
    char *p = NULL;
    char *sbuf;

    if ((self = fopen("/proc/self/status", "r")) != NULL) {
        while (fgets(line, sizeof(line), self)) {
            if (!strncmp(line, "Name:", 5)) {
                strtok_r(line, "\t\n", &sbuf);
                p = strtok_r(NULL, "\t\n", &sbuf);
                break;
            }
        }
        fclose(self);
    }
    return p;
}

// Initialize console if not already done
static void console_ensure_initialized(void) {
    if (console_initialized) return;

    // Set default ident if not set and not using stdio only
    if (console_identity == NULL) {
        console_identity = console_get_default_ident();
    }

    // Initialize syslog if needed
    if (console_channels & CONSOLE_CHANNEL_SYSLOG) {
        openlog(console_identity, 0, console_syslog_facility);
    }

    console_initialized = 1;
}

// Write to kernel message buffer
static void console_write_kmsg(int priority, const char *topic, const char *message) {
    FILE *kmsg = fopen("/dev/kmsg", "w");
    if (kmsg != NULL) {
        fprintf(kmsg, "<%d>", priority);
        if (console_identity) {
            fprintf(kmsg, "%s: ", console_identity);
        }
        fprintf(kmsg, "[%s]: %s\n", topic, message);
        fclose(kmsg);
    }
}

// Write to stdio (stderr)
static void console_write_stdio(const char *topic, const char *message) {
    if (console_identity) {
        fprintf(stderr, "%s: ", console_identity);
    }
    fprintf(stderr, "[%s]: %s\n", topic, message);
}

// Write to syslog
static void console_write_syslog(int priority, const char *topic, const char *message) {
    syslog(priority, "[%s]: %s", topic, message);
}

void console_set_level(ConsoleLevel level) {
    console_level = level;
}

void console_set_channels(int channels) {
    console_channels = channels;
    console_initialized = 0;  // Force re-initialization
}

void console_set_syslog_facility(int facility) {
    console_syslog_facility = facility;
    console_initialized = 0;  // Force re-initialization
}

void console_set_identity(const char *identity) {
    console_identity = identity;
    console_initialized = 0;  // Force re-initialization
}

void console_open(void) {
    console_close();  // Close any existing connections
    console_initialized = 0;
    console_ensure_initialized();
}

void console_close(void) {
    if (!console_initialized) return;

    if (console_channels & CONSOLE_CHANNEL_SYSLOG) {
        closelog();
    }

    console_initialized = 0;
}

void print_log(int priority, const char *topic, const char *format, va_list args) {
    // Format the message first
    char message_buffer[1024];
    vsnprintf(message_buffer, sizeof(message_buffer), format, args);

    // Ensure console is initialized
    console_ensure_initialized();

    // Calculate full syslog priority (facility * 8 + severity)
    int full_priority = (console_syslog_facility << 3) | priority;

    // Write to configured channels
    if (console_channels & CONSOLE_CHANNEL_KMSG) {
        console_write_kmsg(full_priority, topic, message_buffer);
    }

    if (console_channels & CONSOLE_CHANNEL_STDIO) {
        console_write_stdio(topic, message_buffer);
    }

    if (console_channels & CONSOLE_CHANNEL_SYSLOG) {
        console_write_syslog(full_priority, topic, message_buffer);
    }
}

void console_error(Console *csl, const char *format, ...) {
    if (console_level < CONSOLE_LEVEL_ERROR) return;
    va_list args;
    va_start(args, format);
    print_log(CONSOLE_LEVEL_ERROR, csl->topic, format, args);
    va_end(args);
}

void console_warn(Console *csl, const char *format, ...) {
    if (console_level < CONSOLE_LEVEL_WARN) return;
    va_list args;
    va_start(args, format);
    print_log(CONSOLE_LEVEL_WARN, csl->topic, format, args);
    va_end(args);
}

void console_info(Console *csl, const char *format, ...) {
    if (console_level < CONSOLE_LEVEL_INFO) return;
    va_list args;
    va_start(args, format);
    print_log(CONSOLE_LEVEL_INFO, csl->topic, format, args);
    va_end(args);
}

void console_debug(Console *csl, const char *format, ...) {
    if (console_level < CONSOLE_LEVEL_DEBUG) return;
    va_list args;
    va_start(args, format);
    print_log(CONSOLE_LEVEL_DEBUG, csl->topic, format, args);
    va_end(args);
}
