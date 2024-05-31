#include "setup.h"
#include "config.h"
#include "lib/console.h"
#include "lib/requests.h"
#include "state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SETUP_ENDPOINT "/api/nfNode/setup"
#define SETUP_COMPLETE_ENDPOINT "/api/nfNode/setup/complete"

// Backend should handle setup requests that have already been created for this access key
// If no setup request exists, create one
void requestSetup() {
    console(CONSOLE_DEBUG, "Request setup");
    console(CONSOLE_DEBUG, "Access key: %s", state.access_key->key);

    // Build setup URL
    char setup_url[256];
    snprintf(setup_url, sizeof(setup_url), "%s%s", config.main_api, SETUP_ENDPOINT);
    console(CONSOLE_DEBUG, "setup_url: %s", setup_url);

    // Request options
    PostRequestOptions requestSetup = {
        .url = setup_url,
        .key = state.access_key->key,
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

// @TODO: Pending backend implementation
// void checkApprovedSetup()
// {
//     console(CONSOLE_DEBUG, "Not yet implemented - Check if the setup has been approved");
//     console(CONSOLE_DEBUG, "Not yet implemented - Access key: %s", state.access_key->key);
// }

void completeSetup() {
    console(CONSOLE_DEBUG, "complete setup");
    console(CONSOLE_DEBUG, "access key: %s", state.access_key->key);

    // Build setup complete URL
    char setup_complete_url[256];
    snprintf(setup_complete_url, sizeof(setup_complete_url), "%s%s", config.main_api,
             SETUP_COMPLETE_ENDPOINT);
    console(CONSOLE_DEBUG, "setup_complete_url: %s", setup_complete_url);

    PostRequestOptions completeSetupOptions = {
        .url = setup_complete_url,
        .key = state.access_key->key,
        .body = NULL,
        .filePath = NULL,
        .writeFunction = NULL,
        .writeData = NULL,
    };

    performHttpPost(&completeSetupOptions);
}

void setupTask() {
    if (state.setup != 1) {
        console(CONSOLE_DEBUG, "setup is disabled");
        return;
    }

    console(CONSOLE_DEBUG, "setup task");

    if (state.access_status == 0) {
        console(CONSOLE_DEBUG, "requesting setup");
        requestSetup();
    }
}
