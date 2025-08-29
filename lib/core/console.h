// console.h
#ifndef CONSOLE_H
#define CONSOLE_H

#include <syslog.h>

typedef enum {
    CONSOLE_LEVEL_EMERG = LOG_EMERG,
    CONSOLE_LEVEL_ALERT = LOG_ALERT,
    CONSOLE_LEVEL_CRIT = LOG_CRIT,
    CONSOLE_LEVEL_ERROR = LOG_ERR,
    CONSOLE_LEVEL_WARN = LOG_WARNING,
    CONSOLE_LEVEL_NOTICE = LOG_NOTICE,
    CONSOLE_LEVEL_INFO = LOG_INFO,
    CONSOLE_LEVEL_DEBUG = LOG_DEBUG,
} ConsoleLevel;

typedef enum {
    CONSOLE_FACILITY_KERN = LOG_KERN,
    CONSOLE_FACILITY_USER = LOG_USER,
    CONSOLE_FACILITY_DAEMON = LOG_DAEMON,
} ConsoleFacility;

// Define output channels (can be combined with bitwise OR)
typedef enum {
    CONSOLE_CHANNEL_STDIO  = (1 << 0),  // stderr output
    CONSOLE_CHANNEL_SYSLOG = (1 << 1),  // syslog output
    CONSOLE_CHANNEL_KMSG   = (1 << 2),  // kernel message buffer
} ConsoleChannel;

// Define console structure
typedef struct {
    const char *topic;
} Console;

// Global variables
extern ConsoleLevel console_level;
extern int console_channels;
extern int console_syslog_facility;
extern const char *console_identity;

void console_set_level(ConsoleLevel level);
void console_set_channels(int channels);
void console_set_syslog_facility(int facility);
void console_set_identity(const char *identity);
void console_open(void);
void console_close(void);

void console_error(Console *console, const char *format, ...);
void console_warn(Console *console, const char *format, ...);
void console_info(Console *console, const char *format, ...);
void console_debug(Console *console, const char *format, ...);

#endif // CONSOLE_H
