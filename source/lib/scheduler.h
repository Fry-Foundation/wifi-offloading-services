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

    // Flag for periodic tasks
    bool periodic;

    // Interval for periodic tasks
    time_t interval;

    // Detail string for identifying the task
    char detail[64];

    // Pointer to the next task in the list
    struct Task *next;
} Task;

typedef struct Scheduler {
    // Head of the task linked list
    Task *task_list;
} Scheduler;

Task *create_task(time_t execute_at,
                  task_func task_function,
                  void *params,
                  bool periodic,
                  time_t interval,
                  const char *detail);
void schedule_task(Scheduler *scheduler, Task *new_task);
void print_tasks(Scheduler *scheduler);
void run(Scheduler *scheduler);

#endif /* SCHEDULER_H */
