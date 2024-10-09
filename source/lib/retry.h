#include <stdbool.h>

typedef struct {
    bool (*func)(void);
    void (*handle_error)(bool);
    int attempts;
    int delay_seconds;
} RetryConfig;

bool retry(RetryConfig *config);
