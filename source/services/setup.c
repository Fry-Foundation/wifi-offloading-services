#include <stdio.h>
#include "setup.h"
#include "../store/state.h"
#include "../utils/requests.h"

#define REQUEST_SETUP_ENDPOINT "https://api.internal.wayru.tech/api/nfNode/setup"
#define COMPLETE_SETUP_ENDPOINT "https://api.internal.wayru.tech/api/nfNode/setup/complete"

// Backend should handle setup requests that have already been created for this access key
// If no setup request exists, create one
void requestSetup()
{
    printf("[setup] request setup\n");
    
    PostRequestOptions setupRequestOptions = {
        .url = REQUEST_SETUP_ENDPOINT,
        .key = state.accessKey->key,
        .body = NULL,
        .filePath = NULL,
        .writeFunction = NULL,
        .writeData = NULL,
    };

    performHttpPost(&setupRequestOptions);
}

// @TODO: Pending backend implementation
int checkApprovedSetup()
{
    printf("[setup] check if the setup has been approved\n");
}

void completeSetup()
{
    printf("[setup] complete setup\n");

    PostRequestOptions setupRequestOptions = {
        .url = COMPLETE_SETUP_ENDPOINT,
        .key = state.accessKey->key,
        .body = NULL,
        .filePath = NULL,
        .writeFunction = NULL,
        .writeData = NULL,
    };

    performHttpPost(&setupRequestOptions);    
}

void setupTask()
{
    if (state.setup != 1)
    {
        printf("[setup] setup is disabled\n");
        return;
    }

    printf("[setup] setup task\n");

    requestSetup();
    checkApprovedSetup();
    completeSetup();
}