#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Estructura para representar una tarea programada
struct ScheduledTask {
    time_t nextExecutionTime;
    void (*task)();
    int interval;
    int isDone;
};

typedef struct ScheduledTask ScheduledTask;

// Estructura para el planificador de tareas
struct Scheduler {
    ScheduledTask* tasks;
    size_t size;
};

typedef struct Scheduler Scheduler;

// Programa una tarea para ejecutarse en un momento específico
void scheduleAt(Scheduler* sch, time_t time, void (*task)()) {
    sch->size++;
    sch->tasks = realloc(sch->tasks, sch->size * sizeof(ScheduledTask));
    sch->tasks[sch->size - 1].nextExecutionTime = time;
    sch->tasks[sch->size - 1].task = task;
    sch->tasks[sch->size - 1].interval = 0;  // No es periódico
    sch->tasks[sch->size - 1].isDone = 0;
}

// Programa una tarea para ejecutarse periódicamente con un intervalo dado
void scheduleEvery(Scheduler* sch, int interval, void (*task)()) {
    sch->size++;
    sch->tasks = realloc(sch->tasks, sch->size * sizeof(ScheduledTask));
    sch->tasks[sch->size - 1].nextExecutionTime = time(NULL) + interval;
    sch->tasks[sch->size - 1].task = task;
    sch->tasks[sch->size - 1].interval = interval;
    sch->tasks[sch->size - 1].isDone = 0;
}

// Programa una tarea para ejecutarse periódicamente con un intervalo dado
void run(Scheduler* sch) {
    while (1) {
        // Ejecuta las tareas a las que les toca ser ejecutadas
        time_t now = time(NULL);
        size_t i = 0;
        while(i < sch->size) {
            if (now >= sch->tasks[i].nextExecutionTime) {
                sch->tasks[i].task();
                if (sch->tasks[i].interval > 0) {
                    sch->tasks[i].nextExecutionTime += sch->tasks[i].interval;
                } else {
                    // Marcar la tarea para ser eliminada
                    sch->tasks[i].isDone = 1;
                }
            }

            i++; // Pasa a la siguiente tarea
        }

        // Eliminar las tareas que ya fueron ejecutadas
        i = 0;
        while(i < sch->size) {
            if (sch->tasks[i].isDone == 1) {
                for (size_t j = i; j < sch->size - 1; j++) {
                    sch->tasks[j] = sch->tasks[j + 1];
                }

                sch->size--;
                sch->tasks = realloc(sch->tasks, sch->size * sizeof(ScheduledTask));
            } else {
                i++; // Pasa a la siguiente tarea
            }
        }

        usleep(100000); // Esperar 100 ms
    }
}

// Función para ejecutar un script Bash
void task1() {
    // Ruta completa al script Bash a ejecutar
    // char* scriptPath = "../files/get-id.sh";
    char* scriptPath = "/usr/lib/wayru-os-services/get-id.sh";
    printf("Ejecutando script 1: %s\n", scriptPath);

    // Ejecutar el script Bash como un comando del sistema
    char command[100];
    // sprintf(command, "bash %s > ../files/id", scriptPath); // Ruta donde se guardará el resultado
    sprintf(command, "bash %s > /etc/wayru/id", scriptPath); // Ruta donde se guardará el resultado
    system(command);
}

void task2() {
        printf("--3\n");
}

void task3() {
        printf("--5\n");
}

int main() {
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
    scheduleAt(&sch, time(NULL) + 4, task1); // Ejemplo: 3600 segundos = 1 hora

    // Programa la tarea 2 para ejecutarse cada 10 minutos
    scheduleEvery(&sch, 3, task2); // 600 segundos = 10 minutos

    // Ejecuta el planificador
    run(&sch);

    return 0;
}
