#include "lib/core/uloop_scheduler.h"
#include "lib/core/console.h"
#include "apps/agent/services/access_token.h"
#include "apps/agent/services/time_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static Console csl = {
    .topic = "scheduler_test",
};

// Mock structures for testing
typedef struct {
    char *wayru_device_id;
    char *access_key;
} MockRegistration;

typedef struct {
    char *token;
    time_t issued_at_seconds;
    time_t expires_at_seconds;
} MockAccessToken;

typedef struct {
    void (*on_token_refresh)(const char *token, void *context);
    void *context;
} MockAccessTokenCallbacks;

// Mock config values
struct {
    bool dev_env;
    time_t time_sync_interval;
    time_t access_interval;
    char *time_sync_server;
    char *data_path;
    char *accounting_api;
} config = {
    .dev_env = true,  // This will skip time sync in dev mode
    .time_sync_interval = 300,  // 5 minutes
    .access_interval = 3600,    // 1 hour
    .time_sync_server = "pool.ntp.org",
    .data_path = "./test_data",
    .accounting_api = "https://api.example.com"
};

// Simple test callback
void test_callback(void *ctx) {
    static int counter = 0;
    counter++;
    console_info(&csl, "Test callback executed %d times", counter);
    
    if (counter >= 3) {
        console_info(&csl, "Test completed successfully!");
        scheduler_shutdown();
    }
}

int main() {
    console_info(&csl, "Starting scheduler test");
    
    // Initialize scheduler
    scheduler_init();
    
    // Test 1: Basic scheduling
    console_info(&csl, "Test 1: Scheduling basic repeating task");
    task_id_t test_task = schedule_repeating(1000, 2000, test_callback, NULL);
    if (test_task == 0) {
        console_error(&csl, "Failed to schedule test task");
        return 1;
    }
    console_info(&csl, "Scheduled test task with ID %u", test_task);
    
    // Test 2: Time sync service (should skip in dev mode)
    console_info(&csl, "Test 2: Starting time sync service");
    TimeSyncTaskContext *time_sync_context = time_sync_service();
    if (time_sync_context != NULL) {
        console_info(&csl, "Time sync service started");
    } else {
        console_info(&csl, "Time sync service skipped (expected in dev mode)");
    }
    
    // Test 3: Access token service (will fail due to missing dependencies, but should not crash)
    console_info(&csl, "Test 3: Testing access token service initialization");
    MockRegistration reg = {
        .wayru_device_id = "test-device-123",
        .access_key = "test-access-key"
    };
    
    MockAccessToken token = {
        .token = "test-token",
        .issued_at_seconds = time(NULL),
        .expires_at_seconds = time(NULL) + 3600
    };
    
    MockAccessTokenCallbacks callbacks = {
        .on_token_refresh = NULL,
        .context = NULL
    };
    
    // This will likely fail due to network/file dependencies, but shouldn't crash
    AccessTokenTaskContext *access_context = access_token_service(
        (AccessToken*)&token, 
        (Registration*)&reg, 
        (AccessTokenCallbacks*)&callbacks
    );
    
    if (access_context != NULL) {
        console_info(&csl, "Access token service started successfully");
    } else {
        console_info(&csl, "Access token service failed to start (expected due to mock data)");
    }
    
    console_info(&csl, "Starting scheduler main loop");
    
    // Run scheduler (will stop when test_callback calls scheduler_shutdown)
    int result = scheduler_run();
    
    // Cleanup
    if (time_sync_context) {
        clean_time_sync_context(time_sync_context);
    }
    if (access_context) {
        clean_access_token_context(access_context);
    }
    
    console_info(&csl, "Scheduler test completed with result: %d", result);
    return 0;
}