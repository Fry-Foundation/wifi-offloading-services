#ifndef COLLECT_H
#define COLLECT_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// Single-core optimized configuration
#define MAX_LOG_ENTRY_SIZE 512       // Reduced from 1024
#define MAX_BATCH_SIZE 50            // Reduced from 100
#define MAX_QUEUE_SIZE 500           // Reduced from 1000
#define MAX_PROGRAM_SIZE 32          // Reduced from 64
#define MAX_FACILITY_SIZE 16         // Reduced from 32
#define MAX_PRIORITY_SIZE 8          // Reduced from 16

// Timing configuration for single-core
#define BATCH_TIMEOUT_MS 10000       // 10 seconds (longer than multi-core)
#define URGENT_THRESHOLD 400         // 80% of MAX_QUEUE_SIZE
#define HTTP_RETRY_DELAY_MS 2000     // Base retry delay

// Entry pool for memory optimization
#define ENTRY_POOL_SIZE MAX_QUEUE_SIZE

/**
 * Compact log entry structure optimized for memory usage
 */
typedef struct compact_log_entry {
    char message[MAX_LOG_ENTRY_SIZE];
    char program[MAX_PROGRAM_SIZE];
    char facility[MAX_FACILITY_SIZE];
    char priority[MAX_PRIORITY_SIZE];
    uint32_t timestamp;              // 32-bit timestamp instead of time_t
    uint16_t pool_index;             // Index in entry pool
    bool in_use;                     // Pool management flag
} compact_log_entry_t;

/**
 * Simple circular queue for single-threaded access
 */
typedef struct simple_log_queue {
    compact_log_entry_t *entries[MAX_QUEUE_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    uint32_t max_size;
} simple_log_queue_t;

/**
 * HTTP state machine states
 */
typedef enum {
    HTTP_IDLE,
    HTTP_PREPARING,
    HTTP_SENDING,
    HTTP_RETRY_WAIT,
    HTTP_FAILED
} http_state_t;

/**
 * Batch processing context
 */
typedef struct batch_context {
    compact_log_entry_t *entries[MAX_BATCH_SIZE];
    int count;
    time_t created_time;
    int retry_count;
    http_state_t state;
    char *json_payload;
    size_t payload_size;
} batch_context_t;

/**
 * Initialize the log collection system
 * @return 0 on success, negative error code on failure
 */
int collect_init(void);

/**
 * Process any pending batches (called from timer)
 * This replaces the blocking worker thread approach
 * @return 0 on success, negative error code on failure
 */
int collect_process_pending_batches(void);

/**
 * Stop the collection system and cleanup
 */
void collect_cleanup(void);

/**
 * Enqueue a log entry for processing (single-threaded, no locks needed)
 * @param program Program name that generated the log
 * @param message Log message content
 * @param facility Syslog facility
 * @param priority Syslog priority
 * @return 0 on success, negative error code on failure
 */
int collect_enqueue_log(const char *program, const char *message, 
                       const char *facility, const char *priority);

/**
 * Get current queue statistics (single-threaded, no locks needed)
 * @param queue_size Pointer to store current queue size
 * @param dropped_count Pointer to store number of dropped entries
 * @return 0 on success, negative error code on failure
 */
int collect_get_stats(uint32_t *queue_size, uint32_t *dropped_count);

/**
 * Check if collection system is running
 * @return true if system is active, false otherwise
 */
bool collect_is_running(void);

/**
 * Force immediate batch processing (for urgent situations)
 * @return 0 on success, negative error code on failure
 */
int collect_force_batch_processing(void);

/**
 * Get entry from pool (memory optimization)
 * @return pointer to available entry or NULL if pool exhausted
 */
compact_log_entry_t* collect_get_entry_from_pool(void);

/**
 * Return entry to pool (memory optimization)
 * @param entry Entry to return to pool
 */
void collect_return_entry_to_pool(compact_log_entry_t *entry);

/**
 * Get current batch context for state machine processing
 * @return pointer to current batch context
 */
batch_context_t* collect_get_current_batch(void);

/**
 * Advance the HTTP state machine
 * @return 0 to continue, 1 if batch completed, negative on error
 */
int collect_advance_http_state_machine(void);

#endif // COLLECT_H