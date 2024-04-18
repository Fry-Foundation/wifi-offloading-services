// console.c
#include "console.h"
#include <stdio.h>
#include <stdarg.h>

// Global variable to store the current log level
static ConsoleLevel current_console_level = CONSOLE_INFO;

void set_console_level(ConsoleLevel level)
{
  current_console_level = level;
}

void console(ConsoleLevel level, const char *format, ...)
{
   if (level <= current_console_level) {
        // Initialize the arguement list variable
        va_list args;
        va_start(args, format);          

        // Output the level-specific prefix and the formatted message
        switch (level) {
        case CONSOLE_ERROR:
            printf("ERROR: ");
            break;
        case CONSOLE_WARN:
            printf("WARN: ");
            break;
        case CONSOLE_INFO:
            printf("INFO: ");
            break;
        case CONSOLE_DEBUG:
            printf("DEBUG: ");
            break;
        default:
            // Clean up the variable argument list before returning
            va_end(args);              
            // Exit if the level is unknown
            return;        
        }

        // Print the formatted string
        vfprintf(stdout, format, args);
        printf("\n");  

        // Clean up and flush data (print)
        va_end(args); 
        fflush(stdout); 
    }
}

