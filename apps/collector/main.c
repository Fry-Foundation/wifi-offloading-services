#include "core/console.h"
#include "ubus.h"
#include "collect.h"
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <libubox/uloop.h>
#include <sys/sysinfo.h>

static Console csl = {
    .topic = "collector",
};

static volatile bool running = true;
static bool dev_env = false;

// Timer for batch processing
static struct uloop_timeout batch_timer;
static struct uloop_timeout status_timer;

/**
 * Signal handler for graceful shutdown
 */
static void signal_handler(int sig) {
    console_info(&csl, "Received signal %d, shutting down...", sig);
    running = false;
    uloop_end();
}

/**
 * Batch processing timer callback
 */
static void batch_timer_cb(struct uloop_timeout *timeout) {
    if (!running) return;
    
    // Process any pending batches
    collect_process_pending_batches();
    
    // Schedule next batch check
    uloop_timeout_set(&batch_timer, 1000); // Check every second
}

/**
 * Status monitoring timer callback
 */
static void status_timer_cb(struct uloop_timeout *timeout) {
    if (!running) return;
    
    uint32_t queue_size, dropped_count;
    if (collect_get_stats(&queue_size, &dropped_count) == 0) {
        if (dev_env) {
            console_info(&csl, "Status: queue_size=%u, dropped=%u, ubus_connected=%s", 
                       queue_size, dropped_count, 
                       ubus_is_connected() ? "yes" : "no");
        }
        
        // Warn if queue is getting full
        if (queue_size > 400) { // 80% of reduced MAX_QUEUE_SIZE
            console_warn(&csl, "Log queue getting full: %u entries", queue_size);
        }
        
        // Warn about dropped logs
        if (dropped_count > 0) {
            console_warn(&csl, "Dropped %u log entries due to full queue", dropped_count);
        }
    }
    
    // Schedule next status check
    uloop_timeout_set(&status_timer, 30000); // Every 30 seconds
}

/**
 * Process command line arguments
 */
static bool process_command_line_args(int argc, char *argv[]) {
    dev_env = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dev") == 0) {
            dev_env = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [OPTIONS]\n", argv[0]);
            printf("Options:\n");
            printf("  --dev     Run in development mode\n");
            printf("  --help    Show this help message\n");
            return false;
        }
    }

    return true;
}

/**
 * Detect system capabilities
 */
static void detect_system_info(void) {
    int num_cores = get_nprocs();
    console_info(&csl, "Detected %d CPU core(s) - using single-threaded event loop", num_cores);
    
    if (num_cores > 1) {
        console_info(&csl, "Multi-core system detected but using optimized single-core architecture");
    }
}

int main(int argc, char *argv[]) {
    int ret;

    // Process command line arguments
    if (!process_command_line_args(argc, argv)) {
        return 0; // Help was shown, exit normally
    }

    if (dev_env) {
        console_info(&csl, "Collector service started in development mode (single-core optimized)");
    } else {
        console_info(&csl, "Collector service started (single-core optimized)");
    }

    // Detect system information
    detect_system_info();

    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize uloop
    uloop_init();

    // Initialize collection system
    ret = collect_init();
    if (ret < 0) {
        console_error(&csl, "Failed to initialize collection system: %d", ret);
        return 1;
    }

    // Initialize UBUS connection
    ret = ubus_init();
    if (ret < 0) {
        console_error(&csl, "Failed to initialize UBUS: %d", ret);
        collect_cleanup();
        return 1;
    }

    console_info(&csl, "Starting single-threaded event loop...");

    // Set up batch processing timer
    batch_timer.cb = batch_timer_cb;
    uloop_timeout_set(&batch_timer, 1000); // Start in 1 second

    // Set up status monitoring timer
    status_timer.cb = status_timer_cb;
    uloop_timeout_set(&status_timer, 30000); // First status check in 30 seconds

    console_info(&csl, "Collector service running with event-driven architecture");

    // Run the main event loop
    uloop_run();

    console_info(&csl, "Shutting down collector service...");

    // Cancel timers
    uloop_timeout_cancel(&batch_timer);
    uloop_timeout_cancel(&status_timer);

    // Process any final batches
    collect_process_pending_batches();

    // Cleanup
    ubus_cleanup();
    collect_cleanup();
    uloop_done();

    console_info(&csl, "Collector service stopped");
    return 0;
}