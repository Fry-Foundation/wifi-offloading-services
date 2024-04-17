// logger.c
#include "console.h"
#include <stdio.h>

// Global variable to store the current log level
static ConsoleLevel current_console_level = CONSOLE_DEBUG;

void set_console_level(ConsoleLevel level)
{
  current_console_level = level;
}

void console(ConsoleLevel level, const char *message)
{
  if (level <= current_console_level)
  {
    switch (level)
    {
    case CONSOLE_ERROR:
      printf("ERROR: %s\n", message);
      break;
    case CONSOLE_WARN:
      printf("WARN: %s\n", message);
      break;
    case CONSOLE_INFO:
      printf("INFO: %s\n", message);
      break;
    case CONSOLE_DEBUG:
      printf("DEBUG: %s\n", message);
      break;
    default:
      break;
    }

    return;
  }
}
