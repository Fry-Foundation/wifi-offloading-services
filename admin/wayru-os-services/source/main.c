#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Estructura para representar una tarea programada
struct ScheduledTask {
    time_t nextExecutionTime;
    void (*task)();
    int interval;
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
}

// Programa una tarea para ejecutarse periódicamente con un intervalo dado
void scheduleEvery(Scheduler* sch, int interval, void (*task)()) {
    sch->size++;
    sch->tasks = realloc(sch->tasks, sch->size * sizeof(ScheduledTask));
    sch->tasks[sch->size - 1].nextExecutionTime = time(NULL) + interval;
    sch->tasks[sch->size - 1].task = task;
    sch->tasks[sch->size - 1].interval = interval;
}

// Programa una tarea para ejecutarse periódicamente con un intervalo dado
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

// Función para ejecutar un script Bash
void task1() {
    // Ruta completa al script Bash a ejecutar
    char* scriptPath = "/etc/scripts/utils/get-id.sh";
    printf("Ejecutando script 1: %s\n", scriptPath);

    // Ejecutar el script Bash como un comando del sistema
    char command[100];
    sprintf(command, "bash %s > /etc/scripts/utils/identifier/id", scriptPath); // Ruta donde se guardará el resultado
    system(command);
}

void task2() {
        printf("--3\n");
}

int main() {
    Scheduler sch = {NULL, 0};

    // Programa la tarea 1 para ejecutarse en un tiempo determinado (modificar el tiempo aquí)
    scheduleAt(&sch, time(NULL) + 4, task1); // Ejemplo: 3600 segundos = 1 hora

    // Programa la tarea 2 para ejecutarse cada 10 minutos
    scheduleEvery(&sch, 3, task2); // 600 segundos = 10 minutos

    // Ejecuta el planificador
    run(&sch);

    return 0;
}
