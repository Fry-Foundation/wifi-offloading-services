#include "setup.h"
#include "lib/console.h"
#include "lib/http-requests.h"
#include "services/access.h"
#include "services/config.h"
#include "services/device_status.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SETUP_ENDPOINT "/api/nfNode/setup"
#define SETUP_COMPLETE_ENDPOINT "/api/nfNode/setup/complete"

// Backend should handle setup requests that have already been created for this access key
// If no setup request exists, create one
void request_setup() {
    console(CONSOLE_DEBUG, "Request setup");
    console(CONSOLE_DEBUG, "Access key: %s", access_key.public_key);

    // Build setup URL
    char setup_url[256];
    snprintf(setup_url, sizeof(setup_url), "%s%s", config.main_api, SETUP_ENDPOINT);
    console(CONSOLE_DEBUG, "setup_url: %s", setup_url);

    HttpPostOptions setup_options = {
        .url = setup_url,
        .legacy_key = access_key.public_key,
    };

    HttpResult result = http_post(&setup_options);
    if (result.is_error) {
        console(CONSOLE_ERROR, "setup request failed: %s", result.error);
        return;
    }

    if (result.response_buffer == NULL) {
        console(CONSOLE_ERROR, "setup request failed: no response received");
        return;
    }

    console(CONSOLE_DEBUG, "setup request response: %s", result.response_buffer);
}

void setup_task(Scheduler *sch) {
    if (device_status == Unknown) {
        // Schedule setup_task to rerun later
        // The device's status has to be defined beforehand
        console(CONSOLE_DEBUG, "device status is Unknown, rescheduling setup task");
        schedule_task(sch, time(NULL) + config.setup_interval, setup_task, "setup");
    }

    if (device_status == Initial) {
        console(CONSOLE_DEBUG, "requesting setup");
        request_setup();
    }
}

void setup_service(Scheduler *sch) { setup_task(sch); }
