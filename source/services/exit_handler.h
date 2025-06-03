#ifndef EXIT_HANDLER_H
#define EXIT_HANDLER_H

#include <stdbool.h>

typedef void (*cleanup_callback)(void *);

void setup_signal_handlers();

void register_cleanup(cleanup_callback callback, void *data);

void cleanup_and_exit(int exit_code, const char *reason);

void request_cleanup_and_exit(const char *reason);

bool is_shutdown_requested();

const char *get_shutdown_reason();

#endif // EXIT_HANDLER_H
