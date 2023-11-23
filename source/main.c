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
    scheduleEvery(sch, 4, checkPendingRequestTask);
    scheduleAt(sch, time(NULL) + 8, stopHttpServer);
    run(sch);
    return NULL;
}

int main(int argc, char *argv[])
{
    init(argc, argv);

    testGetRequest();

    AccessKey accessKey;
    requestAccessKey(&accessKey);
    writeAccessKey(&accessKey);

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
