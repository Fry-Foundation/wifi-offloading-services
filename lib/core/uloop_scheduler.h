#ifndef ULOOP_SCHEDULER_H
#define ULOOP_SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>

// public handle type
typedef uint32_t task_id_t;

// client-provided function prototype
typedef void (*TaskCallback)(void *ctx);

// Initialize the scheduler (must be called before schedule/cancel/run)
void scheduler_init(void);

// Schedule a one-off task.
// Returns a non-zero task_id, or 0 on failure.
task_id_t schedule_once(uint32_t delay_ms, TaskCallback fn, void *ctx);

// Schedule a repeating task at fixed interval.
// First callback fires after delay_ms, then every interval_ms.
// Returns non-zero task_id, or 0 on failure.
task_id_t schedule_repeating(uint32_t delay_ms, uint32_t interval_ms, TaskCallback fn, void *ctx);

// Cancel a pending task (one-off or repeating).
// Returns true if found and canceled, false otherwise.
bool cancel_task(task_id_t id);

// Start the main loop: blocks until scheduler_shutdown() or uloop_end().
// Returns when all watchers are gone or loop is ended.
int scheduler_run(void);

// Cancel all tasks and stop the loop.
void scheduler_shutdown(void);

#endif /* ULOOP_SCHEDULER_H */