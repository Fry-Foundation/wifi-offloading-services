#include <stdio.h>
#include "setup.h"
#include "../store/state.h"
#include "../utils/requests.h"

#define REQUEST_SETUP_ENDPOINT "https://api.wayru.tech/api/nfNode/setup"
#define COMPLETE_SETUP_ENDPOINT "https://api.wayru.tech/api/nfNode/setup/complete"

// Backend should handle setup requests that have already been created for this access key
// If no setup request exists, create one
void requestSetup()
{
    printf("[setup] Request setup\n");
    printf("[setup] Access key: %s\n", state.accessKey->key);
    
    PostRequestOptions requestSetup = {
        .url = REQUEST_SETUP_ENDPOINT,
        .key = state.accessKey->key,
        .body = NULL,
        .filePath = NULL,
        .writeFunction = NULL,
        .writeData = NULL,
    };

    int result = performHttpPost(&requestSetup);
    if (result == 1) {
        printf("[setup] setup request was a success\n");
    } else {
        printf("[setup] setup request failed\n");
    }
}

// @TODO: Pending backend implementation
int checkApprovedSetup()
{
    printf("[setup] Not yet implemented - Check if the setup has been approved\n");
    printf("[setup] Not yet implemented - Access key: %s\n", state.accessKey->key);
}

void completeSetup()
{
    printf("[setup] Complete setup\n");
    printf("[setup] Access key: %s\n", state.accessKey->key);

    PostRequestOptions completeSetupOptions = {
        .url = COMPLETE_SETUP_ENDPOINT,
        .key = state.accessKey->key,
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

    if (state.accessStatus == 0) {
        requestSetup();
    } else if(state.accessStatus == 2) {
        checkApprovedSetup();
    } else if(state.accessStatus == 3) {
        // Note: We currently complete the setup from the access task
        // since that call receives the updated status value first
        // @TODO: Implement a status endpoint that we can call from here
        // completeSetup();
    }
}