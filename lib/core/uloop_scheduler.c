#include "uloop_scheduler.h"
#include "console.h"
#include <libubox/uloop.h>
#include <libubox/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Console csl = {
    .topic = "uloop_scheduler",
};

// internal task structure
typedef struct Task {
    struct uloop_timeout to; // must be first for container_of()
    task_id_t id;            // unique ID
    TaskCallback fn;         // callback
    void *ctx;               // context pointer
    bool repeating;          // true if auto-reschedules
    uint32_t interval;       // ms for repeating tasks
    struct Task *next;       // linked-list pointer
} Task;

static Task *task_registry = NULL;
static task_id_t next_task_id = 1;
static bool scheduler_initialized = false;

// Forward declarations
static void internal_task_cb(struct uloop_timeout *timeout);
static Task *find_task_by_id(task_id_t id);
static void remove_task_from_registry(Task *task);
static void add_task_to_registry(Task *task);

void scheduler_init(void) {
    if (!scheduler_initialized) {
        uloop_init();
        scheduler_initialized = true;
        console_info(&csl, "uloop scheduler initialized");
    }
    task_registry = NULL;
    next_task_id = 1;
}

static void internal_task_cb(struct uloop_timeout *timeout) {
    Task *t = container_of(timeout, Task, to);

    // Store callback info before potential cleanup
    TaskCallback fn = t->fn;
    void *ctx = t->ctx;
    bool repeating = t->repeating;
    uint32_t interval = t->interval;
    task_id_t task_id = t->id;

    if (repeating) {
        // For repeating tasks, reschedule first
        uloop_timeout_set(&t->to, interval);
    } else {
        // For one-off tasks, remove from registry but don't free yet
        remove_task_from_registry(t);
    }

    // Execute the callback
    if (fn) {
        fn(ctx);
    }

    // Free one-off tasks after callback execution
    if (!repeating) {
        free(t);
    }
}

static Task *find_task_by_id(task_id_t id) {
    Task *current = task_registry;
    while (current != NULL) {
        if (current->id == id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static void add_task_to_registry(Task *task) {
    task->next = task_registry;
    task_registry = task;
}

static void remove_task_from_registry(Task *task) {
    if (task_registry == task) {
        task_registry = task->next;
        return;
    }

    Task *current = task_registry;
    while (current != NULL && current->next != task) {
        current = current->next;
    }

    if (current != NULL) {
        current->next = task->next;
    }
}

task_id_t schedule_once(uint32_t delay_ms, TaskCallback fn, void *ctx) {
    if (!scheduler_initialized) {
        console_error(&csl, "Scheduler not initialized");
        return 0;
    }

    if (!fn) {
        console_error(&csl, "Invalid callback function");
        return 0;
    }

    Task *t = malloc(sizeof(Task));
    if (!t) {
        console_error(&csl, "Failed to allocate memory for task");
        return 0; // allocation failure
    }

    // Handle task ID overflow
    if (next_task_id == 0) {
        next_task_id = 1;
    }

    // Initialize all fields
    memset(t, 0, sizeof(Task));
    t->id = next_task_id++;
    t->fn = fn;
    t->ctx = ctx;
    t->repeating = false;
    t->interval = 0;
    t->next = NULL;
    t->to.cb = internal_task_cb;

    add_task_to_registry(t);
    uloop_timeout_set(&t->to, delay_ms);

    return t->id;
}

task_id_t schedule_repeating(uint32_t delay_ms, uint32_t interval_ms, TaskCallback fn, void *ctx) {
    if (!scheduler_initialized) {
        console_error(&csl, "Scheduler not initialized");
        return 0;
    }

    if (!fn) {
        console_error(&csl, "Invalid callback function");
        return 0;
    }

    if (interval_ms == 0) {
        console_error(&csl, "Invalid interval for repeating task");
        return 0;
    }

    Task *t = malloc(sizeof(Task));
    if (!t) {
        console_error(&csl, "Failed to allocate memory for task");
        return 0; // allocation failure
    }

    // Handle task ID overflow
    if (next_task_id == 0) {
        next_task_id = 1;
    }

    // Initialize all fields
    memset(t, 0, sizeof(Task));
    t->id = next_task_id++;
    t->fn = fn;
    t->ctx = ctx;
    t->repeating = true;
    t->interval = interval_ms;
    t->next = NULL;
    t->to.cb = internal_task_cb;

    add_task_to_registry(t);
    uloop_timeout_set(&t->to, delay_ms);

    return t->id;
}

bool cancel_task(task_id_t id) {
    if (!scheduler_initialized) {
        console_error(&csl, "Scheduler not initialized");
        return false;
    }

    Task *t = find_task_by_id(id);
    if (!t) {
        console_warn(&csl, "Task ID %u not found for cancellation", id);
        return false;
    }

    uloop_timeout_cancel(&t->to);
    remove_task_from_registry(t);
    free(t);

    return true;
}

int scheduler_run(void) {
    if (!scheduler_initialized) {
        console_error(&csl, "Scheduler not initialized");
        return -1;
    }

    console_info(&csl, "Starting scheduler main loop");
    int ret = uloop_run();
    console_info(&csl, "Scheduler main loop ended with code %d", ret);
    uloop_done();
    return ret;
}

void scheduler_shutdown(void) {
    if (!scheduler_initialized) {
        return;
    }

    console_info(&csl, "Shutting down scheduler");

    // Cancel all tasks
    Task *current = task_registry;
    int task_count = 0;
    while (current != NULL) {
        Task *next = current->next;
        uloop_timeout_cancel(&current->to);
        free(current);
        current = next;
        task_count++;
    }
    task_registry = NULL;

    console_info(&csl, "Cancelled %d tasks during shutdown", task_count);
    uloop_end();
}
