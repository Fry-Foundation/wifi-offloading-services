#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <json-c/json.h>
#include "access.h"
#include "../store/config.h"
#include "../utils/requests.h"

#define KEY_FILE "/data/access-key"
#define KEY_FILE_BUFFER_SIZE 256
#define REQUEST_BODY_BUFFER_SIZE 256
#define MAX_KEY_SIZE 256
#define MAX_TIMESTAMP_SIZE 256
#define ACCESS_ENDPOINT "https://api.internal.wayru.tech/api/nfnode/access"

time_t convertToTime_t(const char *timestampStr) {
    long long int epoch = strtoll(timestampStr, NULL, 10);
    return (time_t)epoch;
}

int readAccessKey(AccessKey *accessKey)
{
    printf("[access] reading stored access key\n");

    char keyFile[KEY_FILE_BUFFER_SIZE];
    snprintf(keyFile, sizeof(keyFile), "%s%s", getConfig().basePath, KEY_FILE);

    FILE *file = fopen(keyFile, "r");
    if (file == NULL) {
        // Handle error (e.g., file not found)
        fprintf(stderr, "Failed to open key file.\n");
        return 1;
    }

    char line[256];
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
        else if (strncmp(line, "expires_at", 10) == 0)
        {
            sscanf(line, "expires_at %s", expires_at);
        }
    }

    fclose(file);

    time_t createdAt = convertToTime_t(created_at);
    time_t expiresAt = convertToTime_t(expires_at);  

    strcpy(accessKey->key, public_key);
    accessKey->createdAt = createdAt;
    accessKey->expiresAt = expiresAt;

    return 0;
}

void writeAccessKey(AccessKey *accessKey)
{
    printf("[access] writing new access key\n");

    char keyFile[KEY_FILE_BUFFER_SIZE];
    snprintf(keyFile, sizeof(keyFile), "%s%s", getConfig().basePath, KEY_FILE);


    FILE *file = fopen(keyFile, "w");
    if (file == NULL)
    {
        printf("Unable to open file for writing\n");
        return;
    }

    fprintf(file, "public_key %s\n", accessKey->key);
    fprintf(file, "created_at %ld\n", accessKey->createdAt);
    fprintf(file, "expires_at %ld\n", accessKey->expiresAt);

    fclose(file);
}

size_t processAccessKeyResponse(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t realsize = size * nmemb;
    AccessKey *accessKey = (AccessKey *) userdata;

    // Parse JSON
    struct json_object *parsedResponse;
    struct json_object *publicKey;
    struct json_object *payload;
    struct json_object *iat;
    struct json_object *exp;

    parsedResponse = json_tokener_parse(ptr);
    if (parsedResponse == NULL) {
        // JSON parsing failed
        fprintf(stderr, "Failed to parse JSON\n");
        return realsize;
    }

    // Extract fields
    if (json_object_object_get_ex(parsedResponse, "publicKey", &publicKey) &&
        json_object_object_get_ex(parsedResponse, "payload", &payload) &&
        json_object_object_get_ex(payload, "iat", &iat) &&
        json_object_object_get_ex(payload, "exp", &exp)) {

        accessKey->key = malloc(strlen(json_object_get_string(publicKey)) + 1); // +1 for null-terminator
        strcpy(accessKey->key, json_object_get_string(publicKey));
        accessKey->createdAt = json_object_get_int64(iat);
        accessKey->expiresAt = json_object_get_int64(exp);
    } else {
        fprintf(stderr, "Failed to extract fields\n");
    }

    // Clean up
    json_object_put(parsedResponse);

    return realsize;
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

int requestAccessKey(AccessKey *accessKey)
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
        .filePath = NULL,
        .key = NULL,
        .writeFunction = processAccessKeyResponse,
        .writeData = accessKey
    };

    int resultPost = performHttpPost(&options);
    if (resultPost == 0)
    {
        printf("POST request was a success.\n");
        return 0;
    }
    else
    {
        printf("POST request failed.\n");
        return 1;
    }
};
