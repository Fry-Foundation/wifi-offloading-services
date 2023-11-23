#include <stdio.h>
#include <string.h>
#include "access.h"
#include "../store/config.h"
#include "../utils/requests.h"

#define KEY_FILE "/data/key"
#define KEY_FILE_BUFFER_SIZE 256
#define REQUEST_BODY_BUFFER_SIZE 256
#define MAX_KEY_SIZE 256
#define MAX_TIMESTAMP_SIZE 256
#define ACCESS_ENDPOINT "https://api.internal.wayru.tech/api/nfnode/access"

AccessKey readAccessKey()
{
    char keyFile[KEY_FILE_BUFFER_SIZE];
    snprintf(keyFile, sizeof(keyFile), "%s%s", getConfig().basePath, KEY_FILE);

    FILE *file = fopen(keyFile, "r");
    if (file == NULL) {
        // Handle error (e.g., file not found)
        fprintf(stderr, "Failed to open key file.\n");
        return;
    }

    char line[256]; // Adjust size as needed
    char public_key[MAX_KEY_SIZE];
    char created_at[MAX_TIMESTAMP_SIZE];
    char expires_at[MAX_TIMESTAMP_SIZE];

    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "public_key", 10) == 0)
        {
            sscanf(line, "public_key %s", public_key);
        }
        else if (strncmp(line, "created_at", 10) == 0)
        {
            sscanf(line, "created_at %s", created_at);
        }
        else if (strncmp(line, "expires_At", 10) == 0)
        {
            sscanf(line, "expires_At %s", expires_at);
        }
        // Add more else if blocks for additional lines/fields
    }

    return (AccessKey){
        .key = public_key,
        .createdAt = created_at,
        .expiresAt = expires_at,
    };
}

void writeAccessKey()
{
    // Save the key to the key file in the following format:
    // public_key <key>
    // created_at <timestamp>
    // expires_at <timestamp>    
    printf("[access] writeAccessKey not yet implemented\n");
}

int checkAccessKey()
{
    // Check if the key file exists
    // If it doesn't exist, return 0 to indicate that the key is not valid
    // If it exists, read relevant data
    // Data is stored in the key file in the following format:
    // public_key <key>
    // created_at <timestamp>
    // expires_at <timestamp>
    // If the key is expired, return 0 to indicate that the key is not valid
    // If the key is valid, return 1 to indicate that the key is valid    
    printf("[access] checkAccessKey not yet implemented\n");
    return 0;
}

void requestAccessKey()
{
    printf("[access] Request access key\n");

    char keyFile[KEY_FILE_BUFFER_SIZE];
    snprintf(keyFile, sizeof(keyFile), "%s%s", getConfig().basePath, KEY_FILE);

    char jsonData[REQUEST_BODY_BUFFER_SIZE];
    int written = snprintf(
        jsonData,
        REQUEST_BODY_BUFFER_SIZE,
        "{\n"
        "  \"device_id\": \"%s\",\n"
        "  \"mac\": \"%s\",\n"
        "  \"brand\": \"%s\",\n"
        "  \"model\": \"%s\",\n"
        "  \"os_name\": \"%s\",\n"
        "  \"os_version\": \"%s\",\n"
        "  \"os_services_version\": \"%s\"\n"
        "}",
        getConfig().deviceId,
        getConfig().mac,
        "tp-link",
        getConfig().model,
        "openwrt",
        getConfig().osVersion,
        getConfig().servicesVersion);

    printf("DeviceData -> %s\n", jsonData);
    printf("json length %ld\n", strlen(jsonData));
    printf("written %d\n", written);

    PostRequestOptions options = {
        .url = ACCESS_ENDPOINT,
        .body = jsonData,
        .filePath = keyFile,
        .key = NULL,
    };

    int resultPost = performHttpPost(&options);
    if (resultPost == 0)
    {
        printf("POST request was a success.\n");
    }
    else
    {
        printf("POST request failed.\n");
    }
};
