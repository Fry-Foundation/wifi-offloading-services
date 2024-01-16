#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "setup.h"
#include "../store/state.h"
#include "../utils/requests.h"
#include "../store/config.h"

#define SETUP_ENDPOINT "/api/nfNode/setup"
#define SETUP_COMPLETE_ENDPOINT "/api/nfNode/setup/complete"

// Backend should handle setup requests that have already been created for this access key
// If no setup request exists, create one
void requestSetup()
{
    printf("[setup] Request setup\n");
    printf("[setup] Access key: %s\n", state.access_key->key);

    // Build setup URL
    char setup_url[256];
    snprintf(setup_url, sizeof(setup_url), "%s%s", getConfig().main_api, SETUP_ENDPOINT);
    printf("[setup] setup_url: %s\n", setup_url);

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
    if (result == 1)
    {
        printf("[setup] setup request was a success\n");
    }
    else
    {
        printf("[setup] setup request failed\n");
    }
}

// @TODO: Pending backend implementation
// void checkApprovedSetup()
// {
//     printf("[setup] Not yet implemented - Check if the setup has been approved\n");
//     printf("[setup] Not yet implemented - Access key: %s\n", state.access_key->key);
// }

void completeSetup()
{
    printf("[setup] Complete setup\n");
    printf("[setup] Access key: %s\n", state.access_key->key);

    // Build setup complete URL
    char setup_complete_url[256];
    snprintf(setup_complete_url, sizeof(setup_complete_url), "%s%s", getConfig().main_api, SETUP_COMPLETE_ENDPOINT);
    printf("[setup] setup_complete_url: %s\n", setup_complete_url);

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

void setupTask()
{
    if (state.setup != 1)
    {
        printf("[setup] Setup is disabled\n");
        return;
    }

    printf("[setup] Setup task\n");

    if (state.access_status == 0)
    {
        printf("[setup] Requesting setup\n");
        requestSetup();
    }
}