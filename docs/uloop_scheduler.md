# ULoop Scheduler

A high-performance, event-driven task scheduler built on top of libubox's uloop for OpenWrt/LEDE systems.

## Overview

The ULoop Scheduler provides a modern replacement for traditional polling-based schedulers by leveraging individual `uloop_timeout` watchers for each scheduled task. This approach offers millisecond precision timing and seamless integration with the uloop event system.

## Features

- **Per-task timers**: Each task gets its own `uloop_timeout` for precise scheduling
- **Millisecond precision**: Schedule tasks with millisecond-level accuracy
- **One-off and repeating tasks**: Support for both single-execution and recurring tasks
- **Task cancellation**: Cancel pending tasks by ID before they execute
- **Memory efficient**: Automatic cleanup of completed one-off tasks
- **Thread-safe**: Designed for single-threaded event loop environments
- **Clean shutdown**: Graceful cancellation of all pending tasks

## API Reference

### Initialization

```c
void scheduler_init(void);
```

Initialize the scheduler. Must be called before scheduling any tasks. This also initializes uloop if not already done.

### Task Scheduling

```c
task_id_t schedule_once(uint32_t delay_ms, TaskCallback fn, void *ctx);
```

Schedule a one-time task to execute after `delay_ms` milliseconds.

**Parameters:**
- `delay_ms`: Delay in milliseconds before execution
- `fn`: Callback function to execute
- `ctx`: Context pointer passed to the callback

**Returns:** Task ID (non-zero) on success, 0 on failure

```c
task_id_t schedule_repeating(uint32_t delay_ms, uint32_t interval_ms, 
                            TaskCallback fn, void *ctx);
```

Schedule a repeating task that executes every `interval_ms` milliseconds after an initial delay.

**Parameters:**
- `delay_ms`: Initial delay in milliseconds before first execution
- `interval_ms`: Interval in milliseconds between executions
- `fn`: Callback function to execute
- `ctx`: Context pointer passed to the callback

**Returns:** Task ID (non-zero) on success, 0 on failure

### Task Management

```c
bool cancel_task(task_id_t id);
```

Cancel a pending task before it executes.

**Parameters:**
- `id`: Task ID returned from scheduling functions

**Returns:** `true` if task was found and cancelled, `false` otherwise

### Main Loop

```c
int scheduler_run(void);
```

Start the main event loop. This function blocks until `scheduler_shutdown()` is called or all tasks complete.

**Returns:** uloop return code

```c
void scheduler_shutdown(void);
```

Cancel all pending tasks and stop the main loop. This causes `scheduler_run()` to return.

## Usage Example

```c
#include "core/uloop_scheduler.h"
#include <stdio.h>

// Task callback function
static void my_task(void *ctx) {
    const char *message = (const char *)ctx;
    printf("Task executed: %s\n", message);
}

int main() {
    // Initialize the scheduler
    scheduler_init();
    
    // Schedule a one-off task in 1 second
    task_id_t once_id = schedule_once(1000, my_task, "Hello, World!");
    
    // Schedule a repeating task every 5 seconds, starting in 2 seconds
    task_id_t repeat_id = schedule_repeating(2000, 5000, my_task, "Recurring task");
    
    // Cancel the one-off task (if needed)
    // cancel_task(once_id);
    
    // Start the main loop
    scheduler_run();
    
    return 0;
}
```

## Task Callback Signature

```c
typedef void (*TaskCallback)(void *ctx);
```

Task callbacks receive a single context pointer and should:
- Execute quickly (non-blocking)
- Not call `scheduler_run()` or `scheduler_shutdown()` directly
- Handle their own error conditions gracefully

## Memory Management

- **One-off tasks**: Automatically freed after execution
- **Repeating tasks**: Remain in memory until cancelled or shutdown
- **Context data**: Must remain valid for the lifetime of the task
- **Task IDs**: Reused after tasks complete, so don't store them long-term

## Error Handling

The scheduler handles various error conditions:

- **Memory allocation failures**: Functions return 0 or false
- **Invalid parameters**: Checked and logged appropriately  
- **Double cancellation**: Safe to call, returns false for already-cancelled tasks
- **Shutdown during execution**: All pending tasks are cancelled cleanly

## Integration with Services

Services should be updated to use the new scheduler API:

### Before (Old API)
```c
void my_service(Scheduler *sch) {
    schedule_task(sch, time(NULL) + 300, my_task_func, "my_task", context);
}
```

### After (New API)
```c
void my_service(void) {
    schedule_repeating(300000, 300000, my_task_func, context);
}
```

## Performance Considerations

- **Timer precision**: Limited by system timer resolution (typically 1ms on Linux)
- **Task count**: Efficiently handles dozens to hundreds of concurrent tasks
- **Memory usage**: Approximately 64 bytes per active task
- **CPU overhead**: Minimal - events are driven by kernel timers

## Debugging

The scheduler provides debug logging through the console system:

```c
// Enable debug logging
console_set_level(CONSOLE_DEBUG);
```

This will log task scheduling, execution, and cancellation events.

## Thread Safety

The scheduler is designed for single-threaded use within the uloop event system. Do not call scheduler functions from multiple threads simultaneously.

## Dependencies

- **libubox**: For uloop functionality
- **Standard C library**: For memory management and utilities

## Building

Link with libubox:

```bash
gcc -o myapp myapp.c uloop_scheduler.c console.c -lubox
```

Or use pkg-config:

```bash
gcc -o myapp myapp.c uloop_scheduler.c console.c $(pkg-config --cflags --libs libubox)
```

## Migration from Old Scheduler

See `SCHEDULER_MIGRATION.md` for detailed migration instructions from the previous scheduler implementation.

## License

This scheduler implementation follows the same license as the parent project.