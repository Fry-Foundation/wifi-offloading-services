#include <stdio.h>
#include <stdlib.h>
#include "lib/http-requests.h"
#include "services/config.h"

int internet_check() {
    int status = system("ping -c 1 google.com > /dev/null 2>&1");
    if (status == 0) {
        return 0;
    } else {
        return 1;
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