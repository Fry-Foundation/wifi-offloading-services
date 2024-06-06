#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SLEEP_SECONDS 1

Scheduler scheduler = {NULL};

Task *create_task(time_t execute_at, task_func task_function, void *params, const char *detail) {
    Task *new_task = (Task *)malloc(sizeof(Task));

    // Handle memory allocation failure
    if (!new_task) {
        return NULL;
    }

    new_task->execute_at = execute_at;
    new_task->task_function = task_function;
    new_task->params = params;

    // Char array for detail
    strncpy(new_task->detail, detail, sizeof(new_task->detail) - 1);
    new_task->detail[sizeof(new_task->detail) - 1] = '\0';

    // Next node
    new_task->next = NULL;

    return new_task;
}

void schedule_task(time_t execute_at, task_func task_function, void *params, const char *detail) {
    Task *new_task = create_task(execute_at, task_function, params, detail);

    // Insert task at the beginning of the list if:
    // - Task list is empty
    // - Execution time is earlier than the first task's execution time
    if (!scheduler.task_list || difftime(scheduler.task_list->execute_at, new_task->execute_at) > 0) {
        new_task->next = scheduler.task_list;
        scheduler.task_list = new_task;
        return;
    }

    // Initiliaze a pointer to traverse the task list
    Task *current = scheduler.task_list;

    // Traverse the task list to find the correct position for the new task
    // - The list should be sorted by execution time
    while (current->next && difftime(current->next->execute_at, new_task->execute_at) <= 0) {
        current = current->next;
    }

    // Insert the new task into the list
    new_task->next = current->next;

    // Current task's pointer should point to the new task
    current->next = new_task;
}

void execute_tasks() {
    time_t now = time(NULL);
    printf("Current time is %ld\n", (long)now);

    // Loop to execute all tasks that are due
    while (scheduler.task_list && difftime(scheduler.task_list->execute_at, now) <= 0) {
        // Get the task at the head of the list
        Task *task = scheduler.task_list;

        printf("Task is %s\n", task->detail);
        printf("Task execute_at is %ld\n", (long)task->execute_at);

        // Move the head of the list to the next task
        scheduler.task_list = scheduler.task_list->next;

        // Execute the task's function
        task->task_function(task->params);

        // Free the task memory if it is not periodic
        free(task);
    }
}

void print_tasks() {
    Task *current = scheduler.task_list;
    while (current) {
        printf("Task scheduled at %ld: %s\n", current->execute_at, current->detail);
        current = current->next;
    }
}

void run() {
    while (1) {
        execute_tasks();
        sleep(SLEEP_SECONDS);
    }
}
