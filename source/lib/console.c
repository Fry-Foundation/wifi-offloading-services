// console.c
#include "console.h"
#include <stdarg.h>
#include <stdio.h>

// Global variable to store the current log level
ConsoleLevel console_level = CONSOLE_INFO;

void set_console_level(ConsoleLevel level) {
    console_level = level;
}

void print_log(const char *topic, const char* label, const char *format, va_list args) {
    printf("[%s] ", topic);
    printf("%s: ", label);
    vprintf(format, args);
    printf("\n");
}

void print_error(Console *csl, const char *format, ...) {
    if (console_level < CONSOLE_ERROR) return;
    va_list args;
    va_start(args, format);
    print_log(csl->topic, "error", format, args);
    va_end(args);
}

void print_warn(Console *csl, const char *format, ...) {
    if (console_level < CONSOLE_WARN) return;
    va_list args;
    va_start(args, format);
    print_log(csl->topic, "warn", format, args);
    va_end(args);
}

void print_info(Console *csl, const char *format, ...) {
    if (console_level < CONSOLE_INFO) return;
    va_list args;
    va_start(args, format);
    print_log(csl->topic, "info", format, args);
    va_end(args);
}

void print_debug(Console *csl, const char *format, ...) {
    if (console_level < CONSOLE_DEBUG) return;
    va_list args;
    va_start(args, format);
    print_log(csl->topic, "debug", format, args);
    va_end(args);
}
