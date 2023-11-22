#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pthread.h"
#include "scheduler.h"
#include "server.h"
#include "shared_store.h"
#include "utils/script_runner.h"
#include "utils/generate_id.h"
#include "utils/read_os_version.h"
#include "utils/read_services_version.h"

SharedStore sharedStore = {
    .mode = 0,
    .serverCond = PTHREAD_COND_INITIALIZER,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
};

#include "requests.h"

const char DEV_PATH[] = ".";
const char OPENWRT_PATH[] = "/etc/wayru";
const char *basePath = "";
const char *url = "https://catfact.ninja/fact";

int devMode = 0;
char *scriptsPath = NULL;
char *dataPath = NULL;
char *id = NULL;
char *mac = NULL;
char *model = NULL;
char *osVersion = NULL;
char *servicesVersion = NULL;

void init(int argc, char *argv[])
{
    // Determine if we are running in dev mode
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--dev") == 0)
        {
            devMode = 1;
            break;
        }
    }

    // Set up paths
    if (devMode == 1)
    {
        basePath = DEV_PATH;
    }
    else
    {
        basePath = OPENWRT_PATH;
    }

    snprintf(scriptsPath, sizeof(scriptsPath), "%s%s", basePath, "/scripts");
    snprintf(dataPath, sizeof(dataPath), "%s%s", basePath, "/data");

    printf("basePath: %s\n", basePath);
    printf("scriptsPath: %s\n", scriptsPath);
    printf("dataPath: %s\n", dataPath);

    char scriptFile[256];
    char dataFile[256];

    // Find system data
    // - get os version
    // - get services version
    char *osVersion = readOSVersion();
    printf("OS version: %s\n", osVersion);

    char *servicesVersion = readServicesVersion();
    printf("Services version: %s\n", servicesVersion);

    osVersion = strdup(osVersion);
    servicesVersion = strdup(servicesVersion);

    free(osVersion);
    free(servicesVersion);

    // Set up id
    // - get mac
    // - get model
    // - base64 encode
    snprintf(scriptFile, sizeof(scriptFile), "%s%s", scriptsPath, "/get-mac.sh");
    snprintf(dataFile, sizeof(dataFile), "%s%s", dataPath, "/mac");
    printf("Running mac script: %s\n", scriptFile);
    char *mac = runScript(scriptFile);
    if (strchr(mac, '\n') != NULL)
    {
        mac[strcspn(mac, "\n")] = 0;
    }

    snprintf(scriptFile, sizeof(scriptFile), "%s%s", scriptsPath, "/get-model.sh");
    snprintf(dataFile, sizeof(dataFile), "%s%s", dataPath, "/model");
    printf("Running id script: %s\n", scriptFile);
    char *model = runScript(scriptFile);
    if (strchr(model, '\n') != NULL)
    {
        model[strcspn(model, "\n")] = 0;
    }

    printf("mac: %s\n", mac);
    printf("model: %s\n", model);

    char *encodedId = generateId(mac, model);
    if (encodedId == NULL)
    {
        printf("Failed to generate ID\n");
        exit(1);
    }

    printf("Encoded ID: %s\n", encodedId);

    mac = strdup(mac);
    model = strdup(model);
    id = strdup(encodedId);

    free(mac);
    free(model);
    free(encodedId);
}

void *httpServerRoutine(void *arg)
{
    startHttpServer();
    return NULL;
}

void *schedulerRoutine(void *arg)
{
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

    // Programa la tarea 2 para ejecutarse cada 10 minutos
    scheduleEvery(sch, 4, task2); // 600 segundos = 10 minutos

    run(sch);
    return NULL;
}

int main(int argc, char *argv[])
{
    init(argc, argv);

    printf("Realizando solicitud GET...\n");
    char httpTestFile[256];
    snprintf(httpTestFile, sizeof(httpTestFile), "%s%s", dataPath, "/test");
    int resultGet = performHttpGet(url, httpTestFile);
    if (resultGet == 0)
    {
        printf("Solicitud GET exitosa.\n");
    }
    else
    {
        printf("Fallo en la solicitud GET.\n");
    }

    Scheduler sch = {NULL, 0};

    pthread_t httpServerThread, schedulerThread;

    pthread_create(&httpServerThread, NULL, httpServerRoutine, NULL);
    pthread_create(&schedulerThread, NULL, schedulerRoutine, &sch);
    // --------------------------
    //       --------------------
    //       --------------------

    pthread_join(httpServerThread, NULL);
    pthread_join(schedulerThread, NULL);

    pthread_cond_destroy(&sharedStore.serverCond);
    pthread_mutex_destroy(&sharedStore.mutex);

    // HTTP REQUESTS

    // GET

    return 0;
}
