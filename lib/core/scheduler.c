#include "scheduler.h"
// #include "services/exit_handler.h"
#include "core/console.h"
#include <stdalign.h>
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
    console_info(&csl, "scheduler cleaned up");
}

Task *create_task(time_t execute_at, TaskFunction task_function, const char *detail, void *task_context) {
    Task *new_task = (Task *)malloc(sizeof(Task));

    // Handle memory allocation failure
    if (!new_task) {
        console_error(&csl, "Failed to allocate memory for task");
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
        console_error(&csl, "Failed to create task");
        return;
    }

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
        console_debug(&csl, "No tasks scheduled");
        return;
    }

    console_debug(&csl, "Scheduled tasks:");
    time_t current_time = time(NULL);
    while (current != NULL) {
        int time_left = difftime(current->execute_at, current_time);
        console_debug(&csl, "  %s (in %ds)", current->detail, time_left);
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

    while (sch->head && difftime(sch->head->execute_at, now) <= 0) {
        // Get the task at the head of the list
        Task *task = sch->head;

        console_debug(&csl, "Executing: %s", task->detail);

        // Move the head of the list to the next task
        sch->head = sch->head->next;

        // Execute the task's function
        task->task_function(sch, task->task_context);

        // Free the task memory
        free(task);
    }
}

void run_tasks(Scheduler *sch) {
    while (1) {
        // if (is_shutdown_requested()) {
        //     console_info(&csl, "Shutdown requested, stopping scheduler");
        //     break;
        // }
        execute_tasks(sch);
        sleep(SLEEP_SECONDS);
    }
    // cleanup_and_exit(0, get_shutdown_reason());
}
