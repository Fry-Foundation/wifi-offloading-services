#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "services/init.h"
#include "services/access.h"
#include "services/scheduler.h"
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
    if (resultGet == 1)
    {
        printf("GET request was a success.\n");
    }
    else
    {
        printf("GET request failed.\n");
    }
}

int main(int argc, char *argv[])
{
    init(argc, argv);

    // Request access key to get backend status
    // Note that we disregard the expiration time for now,
    // but the periodic access task does check expiration
    // @TODO: This should probably be part of the access key initialization
    requestAccessKey(state.accessKey);
    writeAccessKey(state.accessKey);
    configureWithAccessStatus(state.accessStatus);
    state.onBoot = 0;

    Scheduler sch = {NULL, 0};

    // Schedule the access task with an interval of 2 minutes
    scheduleEvery(&sch, 120, accessTask);

    // Schedule the setup task for now, and then with an interval of 1 minute
    scheduleAt(&sch, time(NULL), setupTask);
    scheduleEvery(&sch, 60, setupTask);

    // Schedule the accounting task with an interval of 1 minute
    scheduleEvery(&sch, 60, accountingTask);

    run(&sch);

    cleanState();
    cleanConfig();

    return 0;
}
