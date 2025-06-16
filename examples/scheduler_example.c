#include "../lib/core/uloop_scheduler.h"
#include "../lib/core/console.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static Console csl = {
    .topic = "scheduler_example",
};

// Context for tasks
typedef struct {
    int counter;
    const char *name;
} TaskContext;

// Global task IDs for cancellation
static task_id_t heartbeat_task_id = 0;
static task_id_t counter_task_id = 0;

// One-off task example
static void say_hello(void *ctx) {
    const char *message = (const char *)ctx;
    console_info(&csl, "One-off task: %s", message ? message : "Hello, World!");
}

// Repeating task example
static void heartbeat_task(void *ctx) {
    static int beat_count = 0;
    beat_count++;
    console_info(&csl, "Heartbeat #%d", beat_count);
    
    // Cancel after 10 beats
    if (beat_count >= 10) {
        console_info(&csl, "Cancelling heartbeat after 10 beats");
        cancel_task(heartbeat_task_id);
    }
}

// Counter task with context
static void counter_task(void *ctx) {
    TaskContext *task_ctx = (TaskContext *)ctx;
    task_ctx->counter++;
    console_info(&csl, "Counter task '%s': %d", task_ctx->name, task_ctx->counter);
    
    // Cancel after reaching 5
    if (task_ctx->counter >= 5) {
        console_info(&csl, "Counter reached 5, cancelling task");
        cancel_task(counter_task_id);
    }
}

// Cleanup task
static void cleanup_task(void *ctx) {
    console_info(&csl, "Cleanup task executed, shutting down scheduler");
    scheduler_shutdown();
}

// Signal handler for graceful shutdown
static void signal_handler(int sig) {
    console_info(&csl, "Received signal %d, shutting down", sig);
    scheduler_shutdown();
}

int main() {
    console_info(&csl, "Starting uloop scheduler example");
    
    // Install signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize scheduler
    scheduler_init();
    
    // Create context for counter task
    TaskContext *ctx = malloc(sizeof(TaskContext));
    ctx->counter = 0;
    ctx->name = "Example Counter";
    
    // Schedule various tasks
    
    // 1. One-off task in 1 second
    task_id_t hello_id = schedule_once(1000, say_hello, "Hello from one-off task!");
    if (hello_id == 0) {
        console_error(&csl, "Failed to schedule hello task");
        return 1;
    }
    console_info(&csl, "Scheduled one-off hello task (ID: %u)", hello_id);
    
    // 2. Repeating heartbeat every 2 seconds, starting in 2 seconds
    heartbeat_task_id = schedule_repeating(2000, 2000, heartbeat_task, NULL);
    if (heartbeat_task_id == 0) {
        console_error(&csl, "Failed to schedule heartbeat task");
        return 1;
    }
    console_info(&csl, "Scheduled repeating heartbeat task (ID: %u)", heartbeat_task_id);
    
    // 3. Counter task every 3 seconds, starting in 3 seconds
    counter_task_id = schedule_repeating(3000, 3000, counter_task, ctx);
    if (counter_task_id == 0) {
        console_error(&csl, "Failed to schedule counter task");
        return 1;
    }
    console_info(&csl, "Scheduled repeating counter task (ID: %u)", counter_task_id);
    
    // 4. Another one-off task in 5 seconds
    task_id_t goodbye_id = schedule_once(5000, say_hello, "Goodbye from delayed task!");
    if (goodbye_id == 0) {
        console_error(&csl, "Failed to schedule goodbye task");
        return 1;
    }
    console_info(&csl, "Scheduled one-off goodbye task (ID: %u)", goodbye_id);
    
    // 5. Cleanup task in 30 seconds (fallback shutdown)
    task_id_t cleanup_id = schedule_once(30000, cleanup_task, NULL);
    if (cleanup_id == 0) {
        console_error(&csl, "Failed to schedule cleanup task");
        return 1;
    }
    console_info(&csl, "Scheduled cleanup task (ID: %u)", cleanup_id);
    
    // Example of canceling a task
    console_info(&csl, "Cancelling goodbye task before it runs");
    if (cancel_task(goodbye_id)) {
        console_info(&csl, "Successfully cancelled goodbye task");
    } else {
        console_error(&csl, "Failed to cancel goodbye task");
    }
    
    // Start the scheduler
    console_info(&csl, "Starting scheduler main loop");
    int result = scheduler_run();
    
    // Cleanup
    console_info(&csl, "Scheduler finished with code: %d", result);
    free(ctx);
    
    return 0;
}