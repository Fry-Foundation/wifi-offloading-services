#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <json-c/json.h>
#include "access.h"
#include "setup.h"
#include "accounting.h"
#include "../store/config.h"
#include "../store/state.h"
#include "../utils/requests.h"

#define KEY_FILE "/data/access-key"
#define KEY_FILE_BUFFER_SIZE 768
#define REQUEST_BODY_BUFFER_SIZE 256
#define MAX_KEY_SIZE 512
#define MAX_TIMESTAMP_SIZE 256
#define ACCESS_ENDPOINT "https://api.wayru.tech/api/nfnode/access"

time_t convertToTime_t(const char *timestampStr)
{
    long long int epoch = strtoll(timestampStr, NULL, 10);
    return (time_t)epoch;
}

AccessKey *initAccessKey()
{
    AccessKey *accessKey = malloc(sizeof(AccessKey));
    accessKey->key = NULL;
    accessKey->createdAt = 0;
    accessKey->expiresAt = 0;

    readAccessKey(accessKey);

    return accessKey;
}

int readAccessKey(AccessKey *accessKey)
{
    printf("[access] reading stored access key\n");

    char keyFile[KEY_FILE_BUFFER_SIZE];
    snprintf(keyFile, sizeof(keyFile), "%s%s", getConfig().basePath, KEY_FILE);

    FILE *file = fopen(keyFile, "r");
    if (file == NULL)
    {
        // Handle error (e.g., file not found)
        fprintf(stderr, "Failed to open key file.\n");
        return 0;
    }

    char line[512];
    char public_key[MAX_KEY_SIZE];
    char created_at[MAX_TIMESTAMP_SIZE];
    char expires_at[MAX_TIMESTAMP_SIZE];

    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "public_key", 10) == 0)
        {
            // Subtract the length of "public_key " from the total length
            size_t keyLength = strlen(line) - 11;
            accessKey->key = malloc(keyLength + 1);
            if (accessKey->key == NULL)
            {
                perror("Failed to allocate memory for key");
                fclose(file);
                return 0;
            }
            strcpy(accessKey->key, line + 11);
            accessKey->key[keyLength] = '\0';
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

    accessKey->createdAt = createdAt;
    accessKey->expiresAt = expiresAt;

    return 1;
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

void processAccessStatus(char *status)
{
    printf("[access] Processing access status\n");
    if (strcmp(status, "initial") == 0)
    {
        state.accessStatus = 0;
    }
    else if (strcmp(status, "banned") == 0)
    {
        state.accessStatus = 1;
    }
    else if (strcmp(status, "setup-pending") == 0)
    {
        state.accessStatus = 2;
    }
    else if (strcmp(status, "setup-approved") == 0)
    {
        state.accessStatus = 3;
    }
    else if (strcmp(status, "setup-completed") == 0)
    {
        state.accessStatus = 4;
    }
}

size_t processAccessKeyResponse(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t realsize = size * nmemb;
    AccessKey *accessKey = (AccessKey *)userdata;

    // Parse JSON
    struct json_object *parsedResponse;
    struct json_object *publicKey;
    struct json_object *status;
    struct json_object *payload;
    struct json_object *iat;
    struct json_object *exp;

    parsedResponse = json_tokener_parse(ptr);
    if (parsedResponse == NULL)
    {
        // JSON parsing failed
        fprintf(stderr, "Failed to parse JSON\n");
        return realsize;
    }

    // Extract fields
    if (json_object_object_get_ex(parsedResponse, "publicKey", &publicKey) &&
        json_object_object_get_ex(parsedResponse, "status", &status) &&
        json_object_object_get_ex(parsedResponse, "payload", &payload) &&
        json_object_object_get_ex(payload, "iat", &iat) &&
        json_object_object_get_ex(payload, "exp", &exp))
    {
        accessKey->key = malloc(strlen(json_object_get_string(publicKey)) + 1); // +1 for null-terminator
        strcpy(accessKey->key, json_object_get_string(publicKey));
        accessKey->createdAt = json_object_get_int64(iat);
        accessKey->expiresAt = json_object_get_int64(exp);

        // @TODO: Move this logic outside this function
        char *statusValue = malloc(strlen(json_object_get_string(status)) + 1);
        strcpy(statusValue, json_object_get_string(status));
        printf("[access] status: %s\n", statusValue);
        processAccessStatus(statusValue);
    }
    else
    {
        fprintf(stderr, "Failed to extract fields\n");
    }

    // Clean up
    json_object_put(parsedResponse);

    return realsize;
}

int checkAccessKeyExpiration(AccessKey *accessKey)
{
    printf("[access] Checking expiration\n");
    time_t now;
    time(&now);

    if (difftime(now, accessKey->expiresAt) > 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int checkAccessKeyNearExpiration(AccessKey *accessKey)
{
    printf("[access] Checking if key is near expiration\n");
    time_t now;
    time(&now);

    if (difftime(accessKey->expiresAt, now) <= 600)
    {
        printf("[access] Key is near expiration\n");
        return 1;
    }
    else
    {
        printf("[access] Key is not near expiration\n");
        return 0;
    }
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
        "  \"os_services_version\": \"%s\",\n"
        "  \"on_boot\": \"%s\"\n"
        "}",
        getConfig().deviceId,
        getConfig().mac,
        getConfig().brand,
        getConfig().model,
        "wayru-os",
        getConfig().osVersion,
        getConfig().servicesVersion,
        state.onBoot == 1 ? "true" : "false");

    printf("DeviceData -> %s\n", jsonData);
    printf("json length %ld\n", strlen(jsonData));
    printf("written %d\n", written);

    PostRequestOptions options = {
        .url = ACCESS_ENDPOINT,
        .body = jsonData,
        .filePath = NULL,
        .key = NULL,
        .writeFunction = processAccessKeyResponse,
        .writeData = accessKey};

    int resultPost = performHttpPost(&options);
    if (resultPost == 1)
    {
        printf("[access] Request was successful.\n");
        return 1;
    }
    else
    {
        printf("[access] Request failed.\n");
        return 0;
    }
};

void configureWithAccessStatus(int accessStatus)
{
    printf("[access] Configuring with access status\n");
    if (accessStatus == 0)
    {
        printf("[access] Access status is 'initial'\n");
        state.setup = 1;
        state.accounting = 0;

        stopOpenNds();
    }
    else if (accessStatus == 1)
    {
        printf("[access] Access status is 'banned'\n");
        state.setup = 0;
        state.accounting = 0;

        stopOpenNds();
    }
    else if (accessStatus == 2)
    {
        printf("[access] Access status is 'setup-pending'\n");
        state.setup = 1;
        state.accounting = 0;

        stopOpenNds();
    }
    else if (accessStatus == 3)
    {
        printf("[access] Access status is 'setup-approved'\n");
        state.setup = 0;
        state.accounting = 1;

        completeSetup();
        startOpenNds();
    }
    else if (accessStatus == 4)
    {
        printf("[access] Access status is 'setup-completed'\n");
        state.setup = 0;
        state.accounting = 1;

        startOpenNds();
    }
}

void accessTask()
{
    printf("[access] Access task\n");

    int isExpired = checkAccessKeyNearExpiration(state.accessKey);
    if (isExpired == 1 || state.accessKey->key == NULL)
    {
        requestAccessKey(state.accessKey);
        writeAccessKey(state.accessKey);
        configureWithAccessStatus(state.accessStatus);
    }
    else
    {
        printf("[access] key is still valid\n");
    }
}