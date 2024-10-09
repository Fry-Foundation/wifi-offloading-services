#include <lib/retry.h>
#include <lib/result.h>
#include <unistd.h>

bool retry(RetryConfig *config) {
    int attempts = 0;
    bool result = config->retry_func(config->retry_params);
    while (!result && attempts < config->attempts) {
        result = config->retry_func(config->retry_params);
        attempts++;
        sleep(config->delay_seconds);
    }

    return result;
}

