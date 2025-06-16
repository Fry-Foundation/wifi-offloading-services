#include "uloop_scheduler.h"
#include "console.h"
#include <libubox/uloop.h>
#include <libubox/utils.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static Console csl = {
    .topic = "uloop_scheduler",
};

// internal task structure
typedef struct Task {
    struct uloop_timeout to;    // must be first for container_of()
    task_id_t        id;        // unique ID
    TaskCallback     fn;        // callback
    void            *ctx;       // context pointer
    bool             repeating; // true if auto-reschedules
    uint32_t         interval;  // ms for repeating tasks
    struct Task     *next;      // linked-list pointer
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
    
    console_debug(&csl, "Executing task ID %u", t->id);
    
    // Store callback info before potential cleanup
    TaskCallback fn = t->fn;
    void *ctx = t->ctx;
    bool repeating = t->repeating;
    uint32_t interval = t->interval;
    task_id_t task_id = t->id;
    
    if (repeating) {
        // For repeating tasks, reschedule first
        console_debug(&csl, "Rescheduling repeating task ID %u for %u ms", task_id, interval);
        uloop_timeout_set(&t->to, interval);
    } else {
        // For one-off tasks, remove from registry but don't free yet
        console_debug(&csl, "Removing one-off task ID %u from registry", task_id);
        remove_task_from_registry(t);
    }
    
    // Execute the callback
    if (fn) {
        fn(ctx);
    }
    
    // Free one-off tasks after callback execution
    if (!repeating) {
        console_debug(&csl, "Freeing one-off task ID %u", task_id);
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
    console_debug(&csl, "Added task ID %u to registry", task->id);
}

static void remove_task_from_registry(Task *task) {
    if (task_registry == task) {
        task_registry = task->next;
        console_debug(&csl, "Removed task ID %u from registry (head)", task->id);
        return;
    }
    
    Task *current = task_registry;
    while (current != NULL && current->next != task) {
        current = current->next;
    }
    
    if (current != NULL) {
        current->next = task->next;
        console_debug(&csl, "Removed task ID %u from registry", task->id);
    }
}

task_id_t schedule_once(uint32_t delay_ms, TaskCallback fn, void *ctx) {
    console_debug(&csl, "schedule_once called with delay %u ms, fn %p, ctx %p", delay_ms, (void*)fn, ctx);
    
    if (!scheduler_initialized) {
        console_error(&csl, "Scheduler not initialized");
        return 0;
    }
    
    if (!fn) {
        console_error(&csl, "Invalid callback function");
        return 0;
    }
    
    console_debug(&csl, "About to allocate task memory");
    Task *t = malloc(sizeof(Task));
    if (!t) {
        console_error(&csl, "Failed to allocate memory for task");
        return 0; // allocation failure
    }
    console_debug(&csl, "Allocated task at %p", t);
    
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
    
    console_debug(&csl, "Task initialized with ID %u, about to add to registry", t->id);
    add_task_to_registry(t);
    console_debug(&csl, "Task added to registry, about to set uloop timeout");
    uloop_timeout_set(&t->to, delay_ms);
    console_debug(&csl, "uloop_timeout_set completed");
    
    console_debug(&csl, "Scheduled one-off task ID %u with delay %u ms", t->id, delay_ms);
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
    
    console_debug(&csl, "Scheduled repeating task ID %u with delay %u ms, interval %u ms", 
                  t->id, delay_ms, interval_ms);
    return t->id;
}

bool cancel_task(task_id_t id) {
    if (!scheduler_initialized) {
        console_error(&csl, "Scheduler not initialized");
        return false;
    }
    
    Task *t = find_task_by_id(id);
    if (!t) {
        console_debug(&csl, "Task ID %u not found for cancellation", id);
        return false;
    }
    
    console_debug(&csl, "Cancelling task ID %u", id);
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
        console_debug(&csl, "Cancelling task ID %u during shutdown", current->id);
        uloop_timeout_cancel(&current->to);
        free(current);
        current = next;
        task_count++;
    }
    task_registry = NULL;
    
    console_info(&csl, "Cancelled %d tasks during shutdown", task_count);
    uloop_end();
}