#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SLEEP_SECONDS 1

Task *create_task(time_t execute_at, TaskFunction task_function, const char *detail) {
    Task *new_task = (Task *)malloc(sizeof(Task));

    // Handle memory allocation failure
    if (!new_task) {
        return NULL;
    }

    new_task->execute_at = execute_at;
    new_task->task_function = task_function;

    // Char array for detail
    strncpy(new_task->detail, detail, sizeof(new_task->detail) - 1);
    new_task->detail[sizeof(new_task->detail) - 1] = '\0';

    // Next node
    new_task->next = NULL;

    return new_task;
}

void schedule_task(Scheduler *sch, time_t execute_at, TaskFunction task_function, const char *detail) {
    Task *new_task = create_task(execute_at, task_function, detail);

    // Insert task at the beginning of the list if:
    // - Task list is empty
    // - Execution time is earlier than the first task's execution time
    if (!sch->head || difftime(sch->head->execute_at, new_task->execute_at) > 0) {
        new_task->next = sch->head;
        sch->head = new_task;
        return;
    }

    // Initiliaze a pointer to traverse the task list
    Task *current = sch->head;

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

void execute_tasks(Scheduler *sch) {
    time_t now = time(NULL);
    printf("Current time is %ld\n", (long)now);

    // Loop to execute all tasks that are due
    while (sch->head && difftime(sch->head->execute_at, now) <= 0) {
        // Get the task at the head of the list
        Task *task = sch->head;

        printf("Task is %s\n", task->detail);
        printf("Task execute_at is %ld\n", (long)task->execute_at);

        // Move the head of the list to the next task
        sch->head = sch->head->next;

        // Execute the task's function
        task->task_function(sch);

        // Free the task memory if it is not periodic
        free(task);
    }
}

void print_tasks(Scheduler *sch) {
    Task *current = sch->head;
    while (current) {
        printf("Task scheduled at %ld: %s\n", current->execute_at, current->detail);
        current = current->next;
    }
}

void run_tasks(Scheduler *sch) {
    while (1) {
        execute_tasks(sch);
        sleep(SLEEP_SECONDS);
    }
}
