#ifndef EXIT_HANDLER_H
#define EXIT_HANDLER_H

typedef void (*cleanup_callback)(void *);

void setup_signal_handlers();

void register_cleanup(cleanup_callback callback, void *data);

void cleanup_and_exit(int exit_code);

#endif // EXIT_HANDLER_H
