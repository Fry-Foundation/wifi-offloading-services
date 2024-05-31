// console.h
#ifndef CONSOLE_H
#define CONSOLE_H

// Define consoole log levels
typedef enum {
    CONSOLE_NONE = 0,  // No log output
    CONSOLE_ERROR = 1, // Critical errors
    CONSOLE_WARN = 2,  // Warning messages
    CONSOLE_INFO = 3,  // Information messages
    CONSOLE_DEBUG = 4  // Debug messages
} ConsoleLevel;

// Function declarations
void set_console_level(ConsoleLevel level);
void console(ConsoleLevel level, const char *format, ...);

#endif // CONSOLE_H
