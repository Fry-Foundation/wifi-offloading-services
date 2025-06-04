// console.c
#include "console.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Global variable to store the current log level
ConsoleLevel console_level = CONSOLE_INFO;

// Global variable to store the callback function
static ConsoleCallback console_callback = NULL;

void console_set_level(ConsoleLevel level) { console_level = level; }

void console_set_callback(ConsoleCallback callback) { 
    console_callback = callback; 
}

void print_log(const char *topic, const char *label, const char *format, va_list args) {
    // Format the message first
    char message_buffer[1024];
    vsnprintf(message_buffer, sizeof(message_buffer), format, args);
    
    // Print to stdout
    printf("[%s] %s: %s\n", topic, label, message_buffer);
    
    // Call callback if registered
    if (console_callback != NULL) {
        console_callback(topic, label, message_buffer);
    }
}

void console_error(Console *csl, const char *format, ...) {
    if (console_level < CONSOLE_ERROR) return;
    va_list args;
    va_start(args, format);
    print_log(csl->topic, "error", format, args);
    va_end(args);
}

void console_warn(Console *csl, const char *format, ...) {
    if (console_level < CONSOLE_WARN) return;
    va_list args;
    va_start(args, format);
    print_log(csl->topic, "warn", format, args);
    va_end(args);
}

void console_info(Console *csl, const char *format, ...) {
    if (console_level < CONSOLE_INFO) return;
    va_list args;
    va_start(args, format);
    print_log(csl->topic, "info", format, args);
    va_end(args);
}

void console_debug(Console *csl, const char *format, ...) {
    if (console_level < CONSOLE_DEBUG) return;
    va_list args;
    va_start(args, format);
    print_log(csl->topic, "debug", format, args);
    va_end(args);
}
