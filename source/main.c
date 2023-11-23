#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pthread.h"
#include "services/init.h"
#include "services/scheduler.h"
#include "services/server.h"
#include "store/config.h"
#include "store/state.h"
#include "utils/requests.h"
#include "utils/script_runner.h"

#define JSON_BUFFER_SIZE 255

void testGetRequest()
{
    const char TEST_URL[] = "https://catfact.ninja/fact";
    printf("Performing test GET request...\n");
    char httpTestFile[256];
    snprintf(httpTestFile, sizeof(httpTestFile), "%s%s%s", getConfig().basePath, "/data", "/test");
    int resultGet = performHttpGet(TEST_URL, httpTestFile);
    if (resultGet == 0)
    {
        printf("GET request was a success.\n");
    }
    else
    {
        printf("GET request failed.\n");
    }
}

void testGetKey()
{
    printf("Performing test POST key request...\n");
    char httpTestFile[256];
    snprintf(httpTestFile, sizeof(httpTestFile), "%s%s%s", getConfig().basePath, "/data", "/key");

    char jsonString[JSON_BUFFER_SIZE];

    // Building the JSON string
    int written = snprintf(
        jsonString,
        JSON_BUFFER_SIZE,
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

    // Output the JSON string
    printf("DeviceData -> %s\n", jsonString);
    printf("json length %ld\n", strlen(jsonString));
    printf("written %d\n", written);

    int resultPost = performHttpPost(
        "https://api.internal.wayru.tech/api/nfnode/access",
        httpTestFile,
        jsonString);

    if (resultPost == 0)
    {
        printf("POST request was a success.\n");
    }
    else
    {
        printf("POST request failed.\n");
    }
}

void *httpServerRoutine(void *arg)
{
    startHttpServer();
    return NULL;
}

void *schedulerRoutine(void *arg)
{
    Scheduler *sch = (Scheduler *)arg;
    scheduleEvery(sch, 4, checkPendingRequestTask);
    scheduleAt(sch, time(NULL) + 8, stopHttpServer);
    run(sch);
    return NULL;
}

int main(int argc, char *argv[])
{
    init(argc, argv);

    testGetRequest();
    testGetKey();

    Scheduler sch = {NULL, 0};

    pthread_t httpServerThread, schedulerThread;

    pthread_create(&httpServerThread, NULL, httpServerRoutine, NULL);
    pthread_create(&schedulerThread, NULL, schedulerRoutine, &sch);

    pthread_join(httpServerThread, NULL);
    pthread_join(schedulerThread, NULL);

    cleanState();
    cleanConfig();

    return 0;
}
