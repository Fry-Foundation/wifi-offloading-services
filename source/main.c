#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pthread.h"
#include "services/init.h"
#include "services/access.h"
#include "services/scheduler.h"
#include "services/server.h"
#include "store/config.h"
#include "store/state.h"
#include "utils/requests.h"
#include "utils/script_runner.h"

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

void *httpServerRoutine(void *arg)
{
    startHttpServer();
    return NULL;
}

void *schedulerRoutine(void *arg)
{
    Scheduler *sch = (Scheduler *)arg;

    // Schedule the access task for now, and then with an interval of 12 hours
    scheduleAt(sch, time(NULL), accessTask);
    scheduleEvery(sch, 43200, accessTask);

    run(sch);
    return NULL;
}

int main(int argc, char *argv[])
{
    init(argc, argv);

    testGetRequest();
    // int result = readAccessKey(&accessKey);
    // if (result == 1)
    // {
    //     printf("Access key found.\n");
    //     printf("Public key: %s\n", accessKey->key);
    //     printf("Created at: %ld\n", accessKey->createdAt);
    //     printf("Expires at: %ld\n", accessKey->expiresAt);

    //     if (checkAccessKeyExpiration(&accessKey) == 1)
    //     {
    //         printf("Access key expired.\n");
    //         requestAccessKey(&accessKey);
    //         writeAccessKey(&accessKey);
    //     } else {
    //         printf("Access key is still valid.\n");
    //     }
    // }
    // else if (result == 0)
    // {
    //     printf("Access key not found.\n");
    //     requestAccessKey(&accessKey);
    //     writeAccessKey(&accessKey);
    // }

    printf("key: %s\n", state.accessKey->key);

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
