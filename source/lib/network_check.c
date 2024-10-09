#include <stdio.h>
#include <stdlib.h>
#include "lib/http-requests.h"
#include "lib/retry.h"
#include "lib/console.h"
#include "services/config.h"
#include "stdbool.h"
#include <unistd.h>

static Console csl = {
    .topic = "network check",
    .level = CONSOLE_DEBUG,
};

// \brief Check if the device has internet connection with a single ping
// \param host Host to ping
bool ping(void *host) {
    if (host == NULL) return false;
    char *host_str = (char *)host;
    char command[256];
    snprintf(command, sizeof(command), "ping -c 1 %s > /dev/null 2>&1", host_str);
    int status = system(command);
    if (status == 0) {
        print_info(&csl, "Ping to %s successful", host_str);
        return true;
    } else {
        print_error(&csl, "Ping to %s failed", host_str);
        return false;
    }
}

bool internet_check() {
    RetryConfig config;
    config.retry_func = ping;
    config.retry_params = "google.com";
    config.attempts = 5;
    config.delay_seconds = 15;    
    bool result = retry(&config);
    if (result == true) {
        print_info(&csl, "Internet connection is available");
        return true;
    } else {
        print_error(&csl, "No internet connection after %d attempts ... exiting", config.attempts);
        return false;
    }
}

int wayru_check() {
    char url[256];
    snprintf(url, sizeof(url), "%s/health", config.accounting_api);
    HttpGetOptions get_wayru_options = {
        .url = url,
        .bearer_token = NULL,
    };
    HttpResult result = http_get(&get_wayru_options);

    if (result.is_error) {
        return 1;
    } else {
        return 0;
    }

    free(result.response_buffer);

}
