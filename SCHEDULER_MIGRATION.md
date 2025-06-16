# Scheduler Migration Guide

This document explains how to migrate from the old `scheduler.h` to the new `uloop_scheduler.h`.

## Overview

The new scheduler is based on libubox's uloop and provides per-task timers with millisecond precision. Each task gets its own `uloop_timeout` instead of using a single dispatcher loop.

## API Changes

### Old API (scheduler.h)
```c
// Initialization
Scheduler *sch = init_scheduler();
register_cleanup((cleanup_callback)clean_scheduler, sch);

// Scheduling tasks
schedule_task(sch, execute_at, task_function, "task_name", task_context);

// Running
run_tasks(sch);
```

### New API (uloop_scheduler.h)
```c
// Initialization
scheduler_init();
register_cleanup((cleanup_callback)scheduler_shutdown, NULL);

// Scheduling one-off tasks
task_id_t id = schedule_once(delay_ms, callback_function, context);

// Scheduling repeating tasks
task_id_t id = schedule_repeating(initial_delay_ms, interval_ms, callback_function, context);

// Canceling tasks
bool success = cancel_task(id);

// Running
scheduler_run();
```

## Key Differences

1. **Time Format**: Old scheduler used `time_t` (absolute time), new scheduler uses `uint32_t` (milliseconds delay)
2. **Function Signature**: Old `TaskFunction` took `(Scheduler *, void *)`, new `TaskCallback` takes `(void *)`
3. **Task Management**: Old scheduler used linked list with manual sorting, new uses individual uloop timers
4. **Repeating Tasks**: Old scheduler required manual rescheduling, new scheduler handles this automatically

## Service Migration Pattern

### Before (Old Pattern)
```c
// In service header
void time_sync_service(Scheduler *sch);

// In service implementation
static void time_sync_task(Scheduler *sch, void *context) {
    // Do work
    perform_time_sync();
    
    // Reschedule for next run
    schedule_task(sch, time(NULL) + 300, time_sync_task, "time_sync", NULL);
}

void time_sync_service(Scheduler *sch) {
    // Schedule initial run in 60 seconds
    schedule_task(sch, time(NULL) + 60, time_sync_task, "time_sync", NULL);
}
```

### After (New Pattern)
```c
// In service header
void time_sync_service(void);

// In service implementation
static void time_sync_task(void *context) {
    // Do work
    perform_time_sync();
    // No need to reschedule - repeating task handles this automatically
}

void time_sync_service(void) {
    // Schedule repeating task: start in 60s, repeat every 300s
    schedule_repeating(60000, 300000, time_sync_task, NULL);
}
```

## Migration Steps for Services

1. **Update function signatures**: Remove `Scheduler *sch` parameter from service functions
2. **Update task callbacks**: Change from `TaskFunction` to `TaskCallback` signature
3. **Convert time calculations**: Change from absolute `time_t` to relative milliseconds
4. **Use appropriate scheduling**: Choose `schedule_once()` or `schedule_repeating()`
5. **Remove manual rescheduling**: Let repeating tasks handle their own lifecycle
6. **Update main.c calls**: Remove scheduler parameter when calling service init functions

## Example Service Migrations

### Access Token Service
```c
// Old
void access_token_service(Scheduler *sch, AccessToken *token, Registration *reg, AccessTokenCallbacks *callbacks) {
    schedule_task(sch, time(NULL) + 3600, refresh_token_task, "refresh_token", token);
}

// New
void access_token_service(AccessToken *token, Registration *reg, AccessTokenCallbacks *callbacks) {
    schedule_repeating(3600000, 3600000, refresh_token_task, token);
}
```

### Device Status Service
```c
// Old
static void device_status_task(Scheduler *sch, void *context) {
    DeviceStatusContext *ctx = (DeviceStatusContext *)context;
    send_device_status(ctx);
    schedule_task(sch, time(NULL) + 30, device_status_task, "device_status", context);
}

// New
static void device_status_task(void *context) {
    DeviceStatusContext *ctx = (DeviceStatusContext *)context;
    send_device_status(ctx);
    // No rescheduling needed - repeating task
}
```

## Benefits of New Scheduler

1. **Precision**: Millisecond-level timing instead of second-level
2. **Efficiency**: No polling loop, event-driven execution
3. **Integration**: Native uloop integration with other I/O events
4. **Memory**: Per-task cleanup, no global task list management
5. **Flexibility**: Easy task cancellation with task IDs

## Testing Migration

1. Verify all service functions compile without scheduler parameter
2. Check that task callbacks have correct signature
3. Test timing accuracy with millisecond delays
4. Confirm repeating tasks reschedule correctly
5. Test task cancellation functionality
6. Verify clean shutdown cancels all pending tasks

## Common Pitfalls

1. **Time conversion**: Remember to convert seconds to milliseconds (multiply by 1000)
2. **Context lifetime**: Ensure task context remains valid for repeating tasks
3. **Callback blocking**: Keep callbacks short - they run in the main thread
4. **Task ID storage**: Store task IDs if you need to cancel tasks later
5. **Initialization order**: Call `scheduler_init()` before scheduling any tasks