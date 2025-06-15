# Multi-threaded Collector Architecture

This document describes the multi-threaded architecture that was originally implemented for the collector, and serves as a reference for future implementation on multi-core systems.

## Overview

The multi-threaded collector was designed for systems with multiple CPU cores, where true parallelism can be achieved. This architecture separates concerns into dedicated threads for optimal performance.

## Architecture Components

### 1. Main Thread
- **Purpose**: Coordination, monitoring, and signal handling
- **Responsibilities**:
  - Initialize all subsystems
  - Create and manage worker threads
  - Monitor system health and statistics
  - Handle graceful shutdown signals (SIGINT, SIGTERM)
  - Log periodic status updates

### 2. UBUS Thread
- **Purpose**: Handle UBUS events and syslog subscription
- **Responsibilities**:
  - Maintain UBUS connection and subscription to log service
  - Process incoming syslog events with minimal latency
  - Apply quick filtering to reduce processing load
  - Enqueue filtered log entries to shared queue
  - Handle connection recovery and resubscription

### 3. Worker Thread
- **Purpose**: Process batched log entries and send to backend
- **Responsibilities**:
  - Dequeue log entries from shared queue
  - Batch entries based on size or timeout criteria
  - Create JSON payloads for backend submission
  - Execute HTTP requests with retry logic
  - Manage exponential backoff for failed requests

## Thread Communication

### Shared Queue
```c
typedef struct {
    log_entry_t *head;
    log_entry_t *tail;
    uint32_t count;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} threaded_log_queue_t;
```

### Synchronization Primitives
- **Mutex**: `pthread_mutex_t` for queue access protection
- **Condition Variables**: `pthread_cond_t` for efficient thread signaling
- **Atomic Variables**: For lock-free status counters where appropriate

## Threading Model Details

### UBUS Thread Implementation
```c
static void* ubus_thread_func(void* arg) {
    while (running) {
        int ret = ubus_start_loop();
        if (ret < 0) {
            // Handle reconnection with exponential backoff
            handle_ubus_reconnection();
        }
    }
    return NULL;
}
```

### Worker Thread Implementation
```c
static void* worker_thread_func(void* arg) {
    while (!should_stop) {
        pthread_mutex_lock(&queue_mutex);
        
        // Wait for entries or timeout
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1;
        
        while (queue.count == 0 && !should_stop) {
            pthread_cond_timedwait(&queue_cond, &queue_mutex, &timeout);
        }
        
        // Collect batch entries
        collect_batch_entries();
        
        pthread_mutex_unlock(&queue_mutex);
        
        // Process batch with retry logic
        if (should_send_batch()) {
            process_batch_with_retry();
        }
    }
    return NULL;
}
```

## Configuration for Multi-core

### Optimized Settings
```c
#define MAX_LOG_ENTRY_SIZE 1024      // Full size for better throughput
#define MAX_BATCH_SIZE 100           // Larger batches for efficiency
#define MAX_QUEUE_SIZE 1000          // Larger queue for buffering
#define BATCH_TIMEOUT_SECONDS 30     // Shorter timeout for responsiveness
#define MAX_RETRY_ATTEMPTS 3         // More retries for reliability
```

### Thread Priorities
- UBUS Thread: High priority (real-time if possible)
- Worker Thread: Normal priority
- Main Thread: Normal priority

## Memory Management

### Heap-based Log Entries
```c
typedef struct log_entry {
    char message[MAX_LOG_ENTRY_SIZE];
    char program[64];
    char facility[32];
    char priority[16];
    time_t timestamp;
    struct log_entry *next;
} log_entry_t;

// Dynamic allocation for flexibility
log_entry_t* create_log_entry(const char* program, const char* message, 
                              const char* facility, const char* priority) {
    log_entry_t *entry = malloc(sizeof(log_entry_t));
    if (!entry) return NULL;
    
    // Copy data with proper bounds checking
    strncpy(entry->program, program, sizeof(entry->program) - 1);
    // ... etc
    
    return entry;
}
```

### Queue Management with Mutexes
```c
int enqueue_entry_locked(threaded_log_queue_t *q, log_entry_t *entry) {
    pthread_mutex_lock(&q->mutex);
    
    if (q->count >= q->max_size) {
        pthread_mutex_unlock(&q->mutex);
        return -1; // Queue full
    }
    
    // Add to tail
    entry->next = NULL;
    if (q->tail) {
        q->tail->next = entry;
        q->tail = entry;
    } else {
        q->head = entry;
        q->tail = entry;
    }
    
    q->count++;
    
    // Signal waiting worker thread
    pthread_cond_signal(&q->condition);
    
    pthread_mutex_unlock(&q->mutex);
    return 0;
}
```

## Advantages of Multi-threaded Architecture

1. **True Parallelism**: Different threads can run on different CPU cores
2. **Better Responsiveness**: UBUS events processed independently of HTTP operations
3. **Higher Throughput**: Concurrent processing of different pipeline stages
4. **Fault Isolation**: Thread failures don't necessarily crash entire process
5. **Scalability**: Can be extended with additional worker threads

## Disadvantages

1. **Complexity**: Synchronization primitives add complexity
2. **Memory Overhead**: Each thread has its own stack (typically 8MB)
3. **Context Switching**: OS overhead for thread scheduling
4. **Race Conditions**: Potential for subtle threading bugs
5. **Resource Contention**: Threads compete for shared resources

## Performance Characteristics

### Expected Performance on Multi-core
- **Log Processing Rate**: 1000+ logs/second
- **Memory Usage**: ~20-50MB (depending on queue size)
- **CPU Usage**: Distributed across available cores
- **Latency**: Sub-millisecond for UBUS event processing

### Scaling Characteristics
- Linear scaling with number of cores (up to I/O limits)
- Network bandwidth becomes bottleneck before CPU
- Memory usage grows with queue size and entry rate

## When to Use Multi-threaded Architecture

### Suitable Scenarios
- Systems with 2+ CPU cores
- High log volume (>100 logs/second)
- Network latency requires large batching
- System has abundant memory (>128MB available)
- Reliability requirements justify complexity

### Detection Logic
```c
bool should_use_multithreaded_architecture(void) {
    int num_cores = get_nprocs();
    long available_memory = get_available_memory();
    
    return (num_cores >= 2) && 
           (available_memory > 128 * 1024 * 1024) &&
           !force_single_threaded;
}
```

## Migration Strategy

### From Single-threaded to Multi-threaded
1. **Feature Flag**: Add runtime detection and selection
2. **Gradual Rollout**: Test on specific device types first
3. **Performance Monitoring**: Compare metrics between architectures
4. **Fallback Mechanism**: Ability to revert to single-threaded on issues

### Implementation Steps
1. Restore pthread dependencies in CMakeLists.txt
2. Implement threaded queue with proper synchronization
3. Create dedicated thread functions
4. Add thread lifecycle management
5. Implement proper error handling and cleanup
6. Add performance monitoring and metrics

## Testing Considerations

### Multi-threading Specific Tests
- Race condition detection (Helgrind, TSan)
- Deadlock detection
- Thread safety validation
- Performance benchmarking under load
- Memory leak detection across threads
- Signal handling during thread operations

### Load Testing
- Sustained high log rates
- Burst log scenarios
- Network failure recovery
- Memory pressure conditions
- Thread exhaustion scenarios

## Future Enhancements

### Possible Improvements
1. **Thread Pool**: Dynamic worker thread management
2. **NUMA Awareness**: Thread affinity for NUMA systems
3. **Lock-free Queues**: Eliminate mutex overhead
4. **Async HTTP**: Non-blocking HTTP with event loops
5. **Compression**: Compress batches before transmission
6. **Local Buffering**: Persistent storage for network outages

This architecture provides a solid foundation for high-performance log collection on multi-core systems while maintaining the flexibility to optimize for specific deployment scenarios.