// console.h
#ifndef CONSOLE_H
#define CONSOLE_H

// Define console log levels
typedef enum {
    CONSOLE_NONE = 0,  // No log output
    CONSOLE_ERROR = 1, // Critical errors
    CONSOLE_WARN = 2,  // Warning messages
    CONSOLE_INFO = 3,  // Information messages
    CONSOLE_DEBUG = 4  // Debug messages
} ConsoleLevel;

// Define console structure
typedef struct {
    const char *topic;
} Console;

// Define callback function type
// Parameters: topic, level_label, formatted_message
typedef void (*ConsoleCallback)(const char *topic, const char *level_label, const char *message);

// Current level
extern ConsoleLevel console_level;

// Function declarations
void console_set_level(ConsoleLevel level);
void console_set_callback(ConsoleCallback callback);
void console_error(Console *console, const char *format, ...);
void console_warn(Console *console, const char *format, ...);
void console_info(Console *console, const char *format, ...);
void console_debug(Console *console, const char *format, ...);

#endif // CONSOLE_H
