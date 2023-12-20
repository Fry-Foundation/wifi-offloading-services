#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pthread.h"
#include "services/init.h"
#include "services/access.h"
#include "services/scheduler.h"
#include "services/server.h"
#include "services/setup.h"
#include "services/accounting.h"
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

    scheduleAt(sch, time(NULL) + 60, stopOpenNds);
    scheduleAt(sch, time(NULL) + 120, startOpenNds);

    // Schedule the access task for now, and then with an interval of 12 hours
    // @TODO: Dynamic interval
    scheduleAt(sch, time(NULL), accessTask);
    scheduleEvery(sch, 600, accessTask);

    // Schedule the setup task for now, and then with an interval of 1 minute
    // @TODO: Remove / add to task list
    // scheduleEvery(sch, 60, setupTask);

    // Schedule the accounting task with an interval of 1 minute
    // @TODO: Dynamic interval, remove / add to task list
    scheduleEvery(sch, 60, accountingTask);

    run(sch);
    return NULL;
}

int main(int argc, char *argv[])
{
    init(argc, argv);

    testGetRequest();

    char *status = statusOpenNds();
    printf("[accounting] Output of service opennds status: %s\n", status);

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
