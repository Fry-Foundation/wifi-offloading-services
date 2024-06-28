#include "setup.h"
#include "lib/console.h"
#include "lib/requests.h"
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

    // Request options
    PostRequestOptions requestSetup = {
        .url = setup_url,
        .key = access_key.public_key,
        .body = NULL,
        .filePath = NULL,
        .writeFunction = NULL,
        .writeData = NULL,
    };

    int result = performHttpPost(&requestSetup);
    if (result == 1) {
        console(CONSOLE_DEBUG, "setup request was a success");
    } else {
        console(CONSOLE_DEBUG, "setup request failed");
    }
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

    console(CONSOLE_DEBUG, "setup task complete; will not reschedule");
}

void setup_service(Scheduler *sch) { setup_task(sch); }
