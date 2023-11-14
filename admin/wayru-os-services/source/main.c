#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

struct ScheduledTask {
    time_t nextExecutionTime;
    void (*task)();
    int interval;
};

typedef struct ScheduledTask ScheduledTask;

struct Scheduler {
    ScheduledTask* tasks;
    size_t size;
};

typedef struct Scheduler Scheduler;

void scheduleAt(Scheduler* sch, time_t time, void (*task)()) {
    sch->size++;
    sch->tasks = realloc(sch->tasks, sch->size * sizeof(ScheduledTask));
    sch->tasks[sch->size - 1].nextExecutionTime = time;
    sch->tasks[sch->size - 1].task = task;
    sch->tasks[sch->size - 1].interval = 0;  // No es periódico
}

void scheduleEvery(Scheduler* sch, int interval, void (*task)()) {
    sch->size++;
    sch->tasks = realloc(sch->tasks, sch->size * sizeof(ScheduledTask));
    sch->tasks[sch->size - 1].nextExecutionTime = time(NULL) + interval;
    sch->tasks[sch->size - 1].task = task;
    sch->tasks[sch->size - 1].interval = interval;
}

void run(Scheduler* sch) {
    while (1) {
        time_t now = time(NULL);
        for (size_t i = 0; i < sch->size; i++) {
            if (now >= sch->tasks[i].nextExecutionTime) {
                sch->tasks[i].task();
                if (sch->tasks[i].interval > 0) {
                    sch->tasks[i].nextExecutionTime += sch->tasks[i].interval;
                } else {
                    // Eliminar tarea no periódica
                    for (size_t j = i; j < sch->size - 1; j++) {
                        sch->tasks[j] = sch->tasks[j + 1];
                    }
                    sch->size--;
                    sch->tasks = realloc(sch->tasks, sch->size * sizeof(ScheduledTask));
                }
            }
        }
        usleep(100000);  // Esperar 100 ms
    }
}

void task2() {
    // Reemplaza con la ruta completa de tu script Bash
    char* scriptPath = "/home/laura/helloworld/test.sh";
    printf("OK2 ! Running script: %s\n", scriptPath);

    // Ejecutar el script Bash como un comando del sistema
    char command[100];
    sprintf(command, "bash %s", scriptPath);
    int result = system(command);

    // Verificar el resultado de la ejecución del script
    if (result == 0) {
        printf("Script executed successfully.\n");
    } else {
        fprintf(stderr, "Error executing script. Exit code: %d\n", result);
    }
}

void task3() {
    printf("--3\n");
}

void task1(Scheduler* sch) {
    printf("OK1 ! now is   %ld\n", time(NULL));

    scheduleAt(sch, time(NULL) + 1, task2);
    scheduleAt(sch, time(NULL) + 2, task2);
    scheduleAt(sch, time(NULL) + 3, task2);
}

int main() {
    Scheduler sch = {NULL, 0};

    scheduleAt(&sch, time(NULL) + 15, task1);
    scheduleAt(&sch, time(NULL) + 20, task1);
    scheduleAt(&sch, time(NULL) + 25, task1);
    scheduleAt(&sch, time(NULL) + 2, task2);

    scheduleEvery(&sch, 1, task3);

    run(&sch);

    return 0;
}
