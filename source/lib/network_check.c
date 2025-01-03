#include <stdio.h>
#include <stdlib.h>
#include "lib/http-requests.h"
#include "lib/retry.h"
#include "lib/console.h"
#include "services/config.h"
#include "services/diagnostic.h"
#include "stdbool.h"
#include <unistd.h>

static Console csl = {
    .topic = "network check",
};

// \brief Check if the device has internet connection with a single ping. Validating both IPv4 and IPv6
// \param host Host to ping
bool ping(void *params) {
    if (params == NULL) return false;
    char *host = (char *)params;

    char command[256];
    snprintf(command, sizeof(command), "ping -6 -c 1 %s > /dev/null 2>&1", host);
    int status = system(command);

    if (status == 0) {
        print_info(&csl, "Ping to %s successful (IPv6)", host);
        return true;
    } else {
        snprintf(command, sizeof(command), "ping -4 -c 1 %s > /dev/null 2>&1", host);
        status = system(command);

        if (status == 0) {
            print_info(&csl, "Ping to %s successful (IPv4)", host);
            return true;
        } else {
            print_error(&csl, "Ping to %s failed (IPv4 and IPv6)", host);
            return false;
        }
    }
}

bool internet_check() {
    RetryConfig config;
    config.retry_func = ping;
    config.retry_params = "google.com";
    config.attempts = 5;
    config.delay_seconds = 30;
    bool result = retry(&config);
    if (result == true) {
        print_info(&csl, "Internet connection is available");
        update_led_status(true, "Internet check");
        return true;
    } else {
        print_error(&csl, "No internet connection after %d attempts", config.attempts);
        update_led_status(false, "Internet check");
        return false;
    }
}

// \brief Check if the device can reach the wayru accounting API via the /health endpoint
bool wayru_health() {
    char url[256];
    snprintf(url, sizeof(url), "%s/health", config.accounting_api);
    print_info(&csl, "Wayru health url %s", url);
    HttpGetOptions get_wayru_options = {
        .url = url,
        .bearer_token = NULL,
    };
    HttpResult result = http_get(&get_wayru_options);

    free(result.response_buffer);

    if (result.is_error) {
        return false;
    } else {
        return true;
    }


}

bool wayru_check(){
    RetryConfig config;
    config.retry_func = wayru_health;
    config.retry_params = NULL;
    config.attempts = 5;
    config.delay_seconds = 30;
    bool result = retry(&config);
    if (result == true) {
        print_info(&csl, "Wayru is reachable");
        update_led_status(true, "Wayru health check");
        return true;
    } else {
        print_error(&csl, "Wayru is not reachable after %d attempts ... exiting", config.attempts);
        update_led_status(false, "Wayru health check");
        return false;
    }
}
