#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <time.h>

typedef void (*task_func)(void *);

typedef struct Task {
    // Time when the task should be executed
    time_t execute_at;

    // Pointer to the task function
    task_func task_function;

    // Parameters for the task function
    void *params;

    // Detail string for identifying the task
    char detail[64];

    // Pointer to the next task in the list
    struct Task *next;
} Task;

typedef struct Scheduler {
    // Head of the task linked list
    Task *task_list;
} Scheduler;

extern Scheduler scheduler;

void schedule_task(time_t execute_at, task_func task_function, void *params, const char *detail);
void print_tasks();
void run();

#endif /* SCHEDULER_H */
