#include "scheduler.h"
#include <lib/console.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdalign.h>
#include <string.h>
#include <unistd.h>

#define SLEEP_SECONDS 1

Scheduler* init_scheduler() {
    Scheduler *sch = (Scheduler*)malloc(sizeof(Scheduler));
    if (sch != NULL) {
        sch->head = NULL;
    }

    return sch;
}

void clean_scheduler(Scheduler *sch) {
    if (sch == NULL) {
        return;
    }

    Task *current = sch->head;
    while (current != NULL) {
        Task *temp = current;
        current = current->next;
        free(temp);
    }

    free(sch);
}

Task *create_task(time_t execute_at, TaskFunction task_function, const char *detail) {
    console(CONSOLE_DEBUG, "Creating task");
    console(CONSOLE_DEBUG, "With detail: %s", detail);
    Task *new_task = (Task *)malloc(sizeof(Task));

    // Handle memory allocation failure
    if (!new_task) {
        console(CONSOLE_ERROR, "Failed to allocate memory for task");
        return NULL;
    }

    console(CONSOLE_DEBUG, "assigning execute_at");
    new_task->execute_at = execute_at;

    console(CONSOLE_DEBUG, "assigning task_function");
    new_task->task_function = task_function;

    // Char array for detail
    console(CONSOLE_DEBUG, "assigning detail");
    strncpy(new_task->detail, detail, sizeof(new_task->detail) - 1);
    new_task->detail[sizeof(new_task->detail) - 1] = '\0';

    // Next node
    new_task->next = NULL;

    return new_task;
}

void schedule_task(Scheduler *sch, time_t execute_at, TaskFunction task_function, const char *detail) {
    console(CONSOLE_DEBUG, "Schedule task routine ... will create first");
    Task *new_task = create_task(execute_at, task_function, detail);
    if (!new_task) {
        console(CONSOLE_ERROR, "Failed to create task");
        return;
    }

    console(CONSOLE_DEBUG, "Task created with detail: %s", new_task->detail);

    // Insert task at the beginning of the list if:
    // - Task list is empty
    // - Execution time is earlier than the first task's execution time
    if (!sch->head || difftime(sch->head->execute_at, new_task->execute_at) > 0) {
        console(CONSOLE_DEBUG, "Inserting task at the beginning of the list");
        console(CONSOLE_DEBUG, "Scheduler head is at address: %p", (void *)sch->head);
        new_task->next = sch->head;
        console(CONSOLE_DEBUG, "New task next is at address: %p", (void *)new_task->next);
        sch->head = new_task;
        console(CONSOLE_DEBUG, "Scheduler head is now at address: %p", (void *)sch->head);
        exit(0);
        return;
    }

    // Initiliaze a pointer to traverse the task list
    console(CONSOLE_DEBUG, "Traversing the task list to find the correct position for the new task");
    Task *current = sch->head;

    // Traverse the task list to find the correct position for the new task
    // - The list should be sorted by execution time
    while (current->next && difftime(current->next->execute_at, new_task->execute_at) <= 0) {
        console(CONSOLE_DEBUG, "Traversing task list ...");
        current = current->next;
    }

    // Insert the new task into the list
    console(CONSOLE_DEBUG, "Inserting the new task into the list");
    new_task->next = current->next;

    // Current task's pointer should point to the new task
    console(CONSOLE_DEBUG, "Current task's pointer should point to the new task");
    current->next = new_task;
}

void execute_tasks(Scheduler *sch) {
    console(CONSOLE_DEBUG, "-------------------------------------------------");
    console(CONSOLE_DEBUG, "Executing tasks, time is now: %ld", time(NULL));

    time_t now = time(NULL);

    // Loop to execute all tasks that are due
    while (sch->head && difftime(sch->head->execute_at, now) <= 0) {
        // Get the task at the head of the list
        Task *task = sch->head;

        console(CONSOLE_DEBUG, "Executing task: %s", task->detail);
        console(CONSOLE_DEBUG, "Time execute_at: %ld", (long)task->execute_at);

        // Move the head of the list to the next task
        sch->head = sch->head->next;

        // Execute the task's function
        task->task_function(sch);

        // Free the task memory
        console(CONSOLE_DEBUG, "Freeing task memory for task with memory address: %p", (void *)task);
        free(task);
    }
}

void print_tasks(Scheduler *sch) {
    console(CONSOLE_DEBUG, "Printing tasks");
    Task *current = sch->head;
    int task_index = 0;

    while (current) {
        console(CONSOLE_DEBUG, "Processing task %d at address: %p", task_index, (void *)current);

        if (current->detail != NULL) {
            console(CONSOLE_DEBUG, "Task %d detail: %s at address: %p", task_index, current->detail, (void *)current->detail);

            // Isolate the problematic print statement
            console(CONSOLE_DEBUG, "Task scheduled at %ld", current->execute_at);
            console(CONSOLE_DEBUG, "Task detail: %s", current->detail);
            // The combined print statement that might be causing the issue
            console(CONSOLE_DEBUG, "Task scheduled at %ld: %s", current->execute_at, current->detail);
        } else {
            console(CONSOLE_ERROR, "Task %d has a null detail pointer", task_index);
        }

        current = current->next;
        task_index++;
    }
}

void run_tasks(Scheduler *sch) {
    console(CONSOLE_DEBUG, "Running tasks");
    while (1) {
        execute_tasks(sch);
        sleep(SLEEP_SECONDS);
    }
}
