// console.h
#ifndef CONSOLE_H
#define CONSOLE_H

// Define consoole log levels
typedef enum
{
  CONSOLE_NONE,  // No log output
  CONSOLE_ERROR, // Critical errors
  CONSOLE_WARN,  // Warning messages
  CONSOLE_INFO,  // Information messages
  CONSOLE_DEBUG  // Debug messages
} ConsoleLevel;

// Function declarations
void set_console_level(ConsoleLevel level);
void console(ConsoleLevel level, const char *format, ...);

#endif // CONSOLE_H
