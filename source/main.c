#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "script_runner.h"

#include "scheduler.h"

#include "requests.h"

const char DEV_PATH[] = ".";
const char OPENWRT_PATH[] = "/etc/wayru";
const char *basePath = "";
char scriptsPath[256];
char dataPath[256];
int devModeMain = 0;
const char *url = "https://catfact.ninja/fact";
const char *filePath = "./data/test";

int main(int argc, char *argv[])
{
    // Init
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--dev") == 0)
        {
            devModeMain = 1;
            break;
        }
    }

    if (devModeMain == 1)
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

    printf("Realizando solicitud GET...\n");
    int resultGet = performHttpGet(url, filePath);
    if (resultGet == 0)
    {
        printf("Solicitud GET exitosa.\n");
    }
    else
    {
        printf("Fallo en la solicitud GET.\n");
    }

    Scheduler sch = {NULL, 0};

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
    scheduleAt(&sch, time(NULL) + 10, task1); // Ejemplo: 3600 segundos = 1 hora

    // Programa la tarea 2 para ejecutarse cada 10 minutos
    scheduleEvery(&sch, 4, task2); // 600 segundos = 10 minutos

    // Ejecuta el planificador
    run(&sch);

    // HTTP REQUESTS

    // GET

    return 0;
}
