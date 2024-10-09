#include <lib/retry.h>
#include <unistd.h>

bool retry(RetryConfig *config) {
    int attempts = 0;
    bool result = config->func();
    while (result != 0 && attempts < config->attempts) {
        result = config->func();
        config->handle_error(result);
        attempts++;
        sleep(config->delay_seconds);
    }

    return result;
}

