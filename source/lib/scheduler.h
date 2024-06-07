#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <time.h>

typedef struct Scheduler Scheduler;

typedef void (*TaskFunction)(Scheduler *);

typedef struct Task {
    // Time when the task should be executed
    time_t execute_at;

    // Pointer to the task function
    TaskFunction task_function;

    // Scheduler for the task function
    Scheduler *sch;

    // Detail string for identifying the task
    char detail[64];

    // Pointer to the next task in the list
    struct Task *next;
} Task;

typedef struct Scheduler {
    // Head of the task linked list
    Task *head;
} Scheduler;

void schedule_task(Scheduler *sch, time_t execute_at, TaskFunction task_function, const char *detail);
void print_tasks(Scheduler *sch);
void run_tasks(Scheduler *sch);

#endif /* SCHEDULER_H */
