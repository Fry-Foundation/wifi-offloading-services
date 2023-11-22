#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <time.h>

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
