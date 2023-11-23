#include <stdio.h>
#include <string.h>
#include "../store/config.h"
#include "../utils/requests.h"

#define KEY_FILE "/data/key"
#define KEY_FILE_BUFFER_SIZE 256
#define REQUEST_BODY_BUFFER_SIZE 256
#define ACCESS_ENDPOINT "https://api.internal.wayru.tech/api/nfnode/access"

void checkAccessKey(){
    printf("[access] checkAccessKey not yet implemented\n");
}

void getAccessKey()
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

void saveAccessKey() {
    printf("[access] saveAccessKey not yet implemented\n");
}