#include <stdbool.h>

typedef struct {
    bool (*retry_func)(void *);
    void *retry_params;
    int attempts;
    int delay_seconds;
} RetryConfig;

bool retry(RetryConfig *config);
