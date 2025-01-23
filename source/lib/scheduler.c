#include "scheduler.h"
#include "services/exit_handler.h"
#include <lib/console.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SLEEP_SECONDS 1

static Console csl = {
    .topic = "scheduler",
};

Scheduler *init_scheduler() {
    Scheduler *sch = (Scheduler *)malloc(sizeof(Scheduler));
    if (sch != NULL) {
        sch->head = NULL;
    }

    return sch;
}

void clean_scheduler(Scheduler *sch) {
    if (sch == NULL) {
        print_debug(&csl, "no scheduler found, skipping cleanup");
        return;
    }

    Task *current = sch->head;
    while (current != NULL) {
        Task *temp = current;
        current = current->next;

        // Free the task's context if it exists
        if (temp->task_context != NULL) {
            free(temp->task_context);
        }

        free(temp);
    }

    free(sch);
    print_info(&csl, "cleaned scheduler");
}

Task *create_task(time_t execute_at, TaskFunction task_function, const char *detail, void *task_context) {
    Task *new_task = (Task *)malloc(sizeof(Task));

    // Handle memory allocation failure
    if (!new_task) {
        print_error(&csl, "Failed to allocate memory for task");
        return NULL;
    }

    new_task->execute_at = execute_at;
    new_task->task_function = task_function;

    // Char array for detail
    strncpy(new_task->detail, detail, sizeof(new_task->detail) - 1);
    new_task->detail[sizeof(new_task->detail) - 1] = '\0';

    // Set task context
    new_task->task_context = task_context;

    // Next node
    new_task->next = NULL;

    return new_task;
}

void schedule_task(Scheduler *sch,
                   time_t execute_at,
                   TaskFunction task_function,
                   const char *detail,
                   void *task_context) {
    Task *new_task = create_task(execute_at, task_function, detail, task_context);
    if (!new_task) {
        print_error(&csl, "Failed to create task");
        return;
    }

    print_debug(&csl, "Task '%s' created", new_task->detail);

    // Insert task at the beginning of the list if:
    // - Task list is empty
    // - Execution time is earlier than the first task's execution time
    if (!sch->head || difftime(sch->head->execute_at, new_task->execute_at) > 0) {
        new_task->next = sch->head;
        sch->head = new_task;
        print_debug(&csl, "Task '%s' inserted at the beginning of the list", new_task->detail);
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

    print_debug(&csl, "Task '%s' inserted into the list", new_task->detail);
}

int get_task_count(Scheduler *sch) {
    int count = 0;
    Task *current = sch->head;

    while (current != NULL) {
        count++;
        current = current->next;
    }

    return count;
}

void print_tasks(Scheduler *sch) {
    Task *current = sch->head;
    if (current == NULL) {
        print_debug(&csl, "No tasks scheduled");
        return;
    }

    print_debug(&csl, "Tasks scheduled:");
    time_t current_time = time(NULL);
    while (current != NULL) {
        int time_left = difftime(current->execute_at, current_time);
        print_debug(&csl, "- '%s' will run in: %d seconds", current->detail, time_left);
        current = current->next;
    }
}

/*
 * Loop through the task list and execute all tasks that are due.
 *
 * @note The list is sorted by execution time.
 * This means we stop looking when we find a task that is not due because all subsequent tasks will also not be due.
 */
void execute_tasks(Scheduler *sch) {
    time_t now = time(NULL);

    // print_debug(&csl, "-------------------------------------------------");
    // print_debug(&csl, "Executing tasks, time is now: %ld", now);
    // print_debug(&csl, "Task count: %d", get_task_count(sch));
    // print_tasks(sch);
    // print_debug(&csl, "-------------------------------------------------");

    while (sch->head && difftime(sch->head->execute_at, now) <= 0) {
        // Get the task at the head of the list
        Task *task = sch->head;

        print_debug(&csl, "Executing task '%s' with memory address '%p'", task->detail, (void *)task);

        // Move the head of the list to the next task
        sch->head = sch->head->next;

        // Execute the task's function
        task->task_function(sch, task->task_context);

        // Free the task memory
        print_debug(&csl, "Freeing task memory for task with memory address: '%p'", (void *)task);
        free(task);
    }
}

void run_tasks(Scheduler *sch) {
    print_debug(&csl, "Running tasks");
    while (1) {
        if (is_shutdown_requested()) {
            print_info(&csl, "Shutdown requested, stopping task execution");
            break;
        }
        execute_tasks(sch);
        sleep(SLEEP_SECONDS);
    }
    cleanup_and_exit(0);
}
