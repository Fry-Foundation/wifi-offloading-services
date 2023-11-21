#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pthread.h"

#include "script_runner.h"
#include "scheduler.h"
#include "server.h"
#include "shared_store.h"

SharedStore sharedStore = {
    .devMode = 0,
    .scriptsPath = "",
    .dataPath = "",
    .id = "",
    .mac = "",
    .model = "",
    .runServer = 1,
    .serverCond = PTHREAD_COND_INITIALIZER,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
};

const char DEV_PATH[] = ".";
const char OPENWRT_PATH[] = "/etc/wayru";
const char *basePath = "";

void init(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--dev") == 0)
        {
            sharedStore.devMode = 1;
            break;
        }
    }

    if (sharedStore.devMode == 1)
    {
        basePath = DEV_PATH;
    }
    else
    {
        basePath = OPENWRT_PATH;
    }

    snprintf(sharedStore.scriptsPath, sizeof(sharedStore.scriptsPath), "%s%s", basePath, "/scripts");
    snprintf(sharedStore.dataPath, sizeof(sharedStore.dataPath), "%s%s", basePath, "/data");

    printf("basePath: %s\n", basePath);
    printf("scriptsPath: %s\n", sharedStore.scriptsPath);
    printf("dataPath: %s\n", sharedStore.dataPath);
}

void* httpServerRoutine(void *arg) {
    startHttpServer();
    return NULL;
}

void* schedulerRoutine(void *arg) {
    Scheduler *sch = (Scheduler *)arg;

    // Test #1 (mixed)
    // scheduleAt(&sch, time(NULL) + 4, task1);
    // scheduleEvery(&sch, 3, task2);
    // scheduleAt(&sch, time(NULL) + 8, task1);
    // scheduleEvery(&sch, 5, task2);

    // Test #2 (only periodic)
    // scheduleEvery(&sch, 3, task2);
    // scheduleEvery(&sch, 5, task3);

    // Test #3 (only non-periodic)
    // scheduleAt(&sch, time(NULL) + 4, task1);
    // scheduleAt(&sch, time(NULL) + 8, task1);

    // Programa la tarea 1 para ejecutarse en un tiempo determinado (modificar el tiempo aquí)
    scheduleAt(sch, time(NULL) + 10, task1); // Ejemplo: 3600 segundos = 1 hora

    // Programa la tarea 2 para ejecutarse cada 10 minutos
    scheduleEvery(sch, 4, task2); // 600 segundos = 10 minutos

    run(sch);
    return NULL;
}

int main(int argc, char *argv[])
{
    init(argc, argv);
    
    Scheduler sch = {NULL, 0};

    pthread_t httpServerThread, schedulerThread;

    pthread_create(&httpServerThread, NULL, httpServerRoutine, NULL);
    pthread_create(&schedulerThread, NULL, schedulerRoutine, &sch);

    pthread_join(httpServerThread, NULL);
    pthread_join(schedulerThread, NULL);

    pthread_cond_destroy(&sharedStore.serverCond);
    pthread_mutex_destroy(&sharedStore.mutex);    

    return 0;
}
