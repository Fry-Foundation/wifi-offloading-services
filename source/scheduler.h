#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <time.h>

// Tareas periodicas
// - Revisar si hay un pedido de onboarding aprobado para este router (cada 1 minuto) [if mode == 0]
// - Renovar llave publica (1 vez al dia) [sin importar el mode]
// - Enviar updates de accounting (cada 1 minuto) [if mode == 1]

struct ScheduledTask
{
    time_t nextExecutionTime;
    void (*task)();
    int interval;
    int isDone;
};

typedef struct ScheduledTask ScheduledTask;

struct Scheduler
{
    ScheduledTask *tasks;
    size_t size;
};

typedef struct Scheduler Scheduler;

void scheduleAt(Scheduler *sch, time_t time, void (*task)());
void scheduleEvery(Scheduler *sch, int interval, void (*task)());
void run(Scheduler *sch);
void task2();
void task3();

#endif /* SCHEDULER_H */
