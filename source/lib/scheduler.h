#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdalign.h>
#include <stdbool.h>
#include <time.h>

#define SCHEDULER_DETAIL_SIZE 64

typedef struct Scheduler Scheduler;

typedef void (*TaskFunction)(Scheduler *, void *);

typedef struct __attribute__((aligned(8))) Task {
    // Time when the task should be executed
    time_t execute_at;

    // Pointer to the task function
    TaskFunction task_function;

    // Detail string for identifying the task
    char detail[SCHEDULER_DETAIL_SIZE];

    // Pointer to the next task in the list
    struct Task *next;

    // Pointer to the context data for the task
    void *task_context;
} Task;

typedef struct Scheduler {
    // Head of the task linked list
    Task *head;
} Scheduler;

Scheduler *init_scheduler();
void clean_scheduler(Scheduler *sch);
void schedule_task(Scheduler *sch, time_t execute_at, TaskFunction task_function, const char *detail, void *task_context);
void print_tasks(Scheduler *sch);
void run_tasks(Scheduler *sch);

#endif /* SCHEDULER_H */
