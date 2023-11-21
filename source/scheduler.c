#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "scheduler.h"

#include "shared_store.h"

// Programa una tarea para ejecutarse en un momento específico
void scheduleAt(Scheduler *sch, time_t time, void (*task)())
{
    sch->size++;
    sch->tasks = realloc(sch->tasks, sch->size * sizeof(ScheduledTask));
    sch->tasks[sch->size - 1].nextExecutionTime = time;
    sch->tasks[sch->size - 1].task = task;
    sch->tasks[sch->size - 1].interval = 0; // No es periódico
    sch->tasks[sch->size - 1].isDone = 0;
}

// Programa una tarea para ejecutarse periódicamente con un intervalo dado
void scheduleEvery(Scheduler *sch, int interval, void (*task)())
{
    sch->size++;
    sch->tasks = realloc(sch->tasks, sch->size * sizeof(ScheduledTask));
    sch->tasks[sch->size - 1].nextExecutionTime = time(NULL) + interval;
    sch->tasks[sch->size - 1].task = task;
    sch->tasks[sch->size - 1].interval = interval;
    sch->tasks[sch->size - 1].isDone = 0;
}

// Programa una tarea para ejecutarse periódicamente con un intervalo dado
void run(Scheduler *sch)
{
    while (1)
    {
        // Ejecuta las tareas a las que les toca ser ejecutadas
        time_t now = time(NULL);
        size_t i = 0;
        while (i < sch->size)
        {
            if (now >= sch->tasks[i].nextExecutionTime)
            {
                sch->tasks[i].task();
                if (sch->tasks[i].interval > 0)
                {
                    sch->tasks[i].nextExecutionTime += sch->tasks[i].interval;
                }
                else
                {
                    // Marcar la tarea para ser eliminada
                    sch->tasks[i].isDone = 1;
                }
            }

            i++; // Pasa a la siguiente tarea
        }

        // Eliminar las tareas que ya fueron ejecutadas
        i = 0;
        while (i < sch->size)
        {
            if (sch->tasks[i].isDone == 1)
            {
                for (size_t j = i; j < sch->size - 1; j++)
                {
                    sch->tasks[j] = sch->tasks[j + 1];
                }

                sch->size--;
                sch->tasks = realloc(sch->tasks, sch->size * sizeof(ScheduledTask));
            }
            else
            {
                i++; // Pasa a la siguiente tarea
            }
        }

        usleep(100000); // Esperar 100 ms
    }
}

void task1()
{
    // Set up the script and data (result) file paths
    char scriptFile[256];
    char dataFile[256];

    pthread_mutex_lock(&sharedStore.mutex);
    snprintf(scriptFile, sizeof(scriptFile), "%s%s", &sharedStore.scriptsPath, "/get-id.sh");
    snprintf(dataFile, sizeof(dataFile), "%s%s", &sharedStore.dataPath, "/id");
    pthread_mutex_unlock(&sharedStore.mutex);

    if (&sharedStore.devMode)
    {
        printf("Running script: %s\n", scriptFile);
        runScriptAndSaveOutput(scriptFile, dataFile);
    }
    else
    {
        // Execute a bash script as a system command
        // @TODO: Review whether this can run on OpenWrt since bash is not installed by default, they use ash
        char command[100];
        sprintf(command, "bash %s > %s", scriptFile, dataFile);
        system(command);
    }
}

void task2()
{
    printf("--3\n");
}

void task3()
{
    printf("--5\n");
}