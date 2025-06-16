#include "../services/ubus_client.h"
#include "core/console.h"
#include "core/scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static Console csl = {
    .topic = "ubus_example",
};

// Example service context
typedef struct {
    UbusClient *ubus_client;
    char *service_name;
    int check_interval;
    bool monitor_enabled;
} ExampleServiceContext;

// Task context for scheduler integration
typedef struct {
    ExampleServiceContext *service_ctx;
    Scheduler *scheduler;
} ExampleTaskContext;

/**
 * Example: Query wayru-agent device info and log it
 */
static void query_device_info(UbusClient *client) {
    console_info(&csl, "Querying wayru-agent device info...");
    
    UbusResponse *response = ubus_client_call(client, "wayru-agent", "get_device_info", NULL);
    
    if (response && response->success) {
        // Extract specific fields
        const char *device_id = ubus_response_get_string(response, "device_id");
        const char *os_version = ubus_response_get_string(response, "os_version");
        const char *mac = ubus_response_get_string(response, "mac");
        
        console_info(&csl, "Device Info - ID: %s, OS: %s, MAC: %s", 
                    device_id ? device_id : "unknown",
                    os_version ? os_version : "unknown", 
                    mac ? mac : "unknown");
    } else {
        console_error(&csl, "Failed to get device info: %s", 
                     response ? response->error_message : "no response");
    }
    
    if (response) {
        ubus_response_free(response);
    }
}

/**
 * Example: Check if access token is valid
 */
static bool check_access_token_validity(UbusClient *client) {
    console_debug(&csl, "Checking access token validity...");
    
    UbusResponse *response = ubus_client_call(client, "wayru-agent", "get_access_token", NULL);
    
    bool valid = false;
    if (response && response->success) {
        valid = ubus_response_get_bool(response, "valid", false);
        
        if (valid) {
            console_info(&csl, "Access token is valid");
        } else {
            console_warn(&csl, "Access token is invalid or expired");
        }
    } else {
        console_error(&csl, "Failed to check access token: %s",
                     response ? response->error_message : "no response");
    }
    
    if (response) {
        ubus_response_free(response);
    }
    
    return valid;
}

/**
 * Example: Monitor wayru-agent service status
 */
static void monitor_agent_status(UbusClient *client) {
    console_debug(&csl, "Monitoring wayru-agent status...");
    
    // First, ping the service
    bool available = ubus_client_ping_service(client, "wayru-agent");
    if (!available) {
        console_error(&csl, "wayru-agent service is not available");
        return;
    }
    
    // Get detailed status
    UbusResponse *response = ubus_client_call(client, "wayru-agent", "get_status", NULL);
    
    if (response && response->success) {
        bool running = ubus_response_get_bool(response, "running", false);
        bool token_available = ubus_response_get_bool(response, "access_token_available", false);
        bool device_info_available = ubus_response_get_bool(response, "device_info_available", false);
        
        console_info(&csl, "Agent Status - Running: %s, Token: %s, Device Info: %s",
                    running ? "yes" : "no",
                    token_available ? "available" : "unavailable",
                    device_info_available ? "available" : "unavailable");
        
        // Take action based on status
        if (!token_available) {
            console_warn(&csl, "Access token not available - authentication may be required");
        }
        
        if (!device_info_available) {
            console_warn(&csl, "Device info not available - initialization may be incomplete");
        }
    } else {
        console_error(&csl, "Failed to get agent status: %s",
                     response ? response->error_message : "no response");
    }
    
    if (response) {
        ubus_response_free(response);
    }
}

/**
 * Example: Asynchronous callback for service monitoring
 */
static void async_status_callback(UbusResponse *response, void *user_data) {
    const char *service_name = (const char *)user_data;
    
    if (response && response->success) {
        console_info(&csl, "Async status check for %s: SUCCESS", service_name);
        console_info(&csl, "Response: %s", response->json_response);
    } else {
        console_error(&csl, "Async status check for %s: FAILED - %s", 
                     service_name,
                     response ? response->error_message : "no response");
    }
}

/**
 * Example: Discover and interact with other services
 */
static void discover_services(UbusClient *client) {
    console_info(&csl, "Discovering available UBUS services...");
    
    UbusResponse *response = ubus_client_list_services(client);
    
    if (response && response->success) {
        console_info(&csl, "Available services:");
        console_info(&csl, "%s", response->json_response);
        
        // Example: Check if specific services are available
        const char *interesting_services[] = {
            "system",
            "network", 
            "wireless",
            "wayru-agent"
        };
        
        int num_services = sizeof(interesting_services) / sizeof(interesting_services[0]);
        for (int i = 0; i < num_services; i++) {
            bool available = ubus_client_ping_service(client, interesting_services[i]);
            console_info(&csl, "Service '%s': %s", 
                        interesting_services[i], 
                        available ? "AVAILABLE" : "NOT AVAILABLE");
        }
    } else {
        console_error(&csl, "Failed to list services: %s",
                     response ? response->error_message : "no response");
    }
    
    if (response) {
        ubus_response_free(response);
    }
}

/**
 * Example: Call multiple services in sequence
 */
static void multi_service_example(UbusClient *client) {
    console_info(&csl, "Multi-service interaction example...");
    
    // 1. Check agent status
    monitor_agent_status(client);
    
    // 2. Get device info
    query_device_info(client);
    
    // 3. Check token validity
    bool token_valid = check_access_token_validity(client);
    
    // 4. Make decision based on results
    if (token_valid) {
        console_info(&csl, "All checks passed - proceeding with operation");
        
        // Example: Get registration info if token is valid
        UbusResponse *reg_response = ubus_client_call(client, "wayru-agent", "get_registration", NULL);
        if (reg_response && reg_response->success) {
            const char *device_id = ubus_response_get_string(reg_response, "wayru_device_id");
            console_info(&csl, "Registered device ID: %s", device_id ? device_id : "unknown");
        }
        
        if (reg_response) {
            ubus_response_free(reg_response);
        }
    } else {
        console_warn(&csl, "Token invalid - skipping authenticated operations");
    }
}

/**
 * Example scheduler task for periodic UBUS monitoring
 */
static void example_monitoring_task(Scheduler *sch, void *context) {
    ExampleTaskContext *task_ctx = (ExampleTaskContext *)context;
    
    if (!task_ctx || !task_ctx->service_ctx) {
        console_error(&csl, "Invalid task context");
        return;
    }
    
    ExampleServiceContext *service_ctx = task_ctx->service_ctx;
    
    if (!service_ctx->monitor_enabled) {
        // Schedule next check but don't execute
        schedule_task(sch, time(NULL) + service_ctx->check_interval,
                      example_monitoring_task, "example_monitoring", context);
        return;
    }
    
    console_debug(&csl, "Running periodic UBUS monitoring task");
    
    // Check if UBUS client is still connected
    if (!ubus_client_is_connected(service_ctx->ubus_client)) {
        console_error(&csl, "UBUS client disconnected - attempting reconnect");
        
        // Cleanup old client
        ubus_client_cleanup(service_ctx->ubus_client);
        
        // Try to reconnect
        service_ctx->ubus_client = ubus_client_init(5000);
        if (!service_ctx->ubus_client) {
            console_error(&csl, "Failed to reconnect UBUS client");
            goto schedule_next;
        }
    }
    
    // Perform monitoring tasks
    monitor_agent_status(service_ctx->ubus_client);
    
    // Example async call
    ubus_client_call_async(service_ctx->ubus_client, "wayru-agent", "get_status", NULL,
                          async_status_callback, (void *)service_ctx->service_name);

schedule_next:
    // Schedule next execution
    schedule_task(sch, time(NULL) + service_ctx->check_interval,
                  example_monitoring_task, "example_monitoring", context);
}

/**
 * Initialize the example service with UBUS integration
 */
ExampleServiceContext *init_example_service(const char *service_name, int check_interval) {
    console_info(&csl, "Initializing example service: %s", service_name);
    
    ExampleServiceContext *ctx = malloc(sizeof(ExampleServiceContext));
    if (!ctx) {
        console_error(&csl, "Failed to allocate service context");
        return NULL;
    }
    
    memset(ctx, 0, sizeof(ExampleServiceContext));
    
    // Initialize UBUS client
    ctx->ubus_client = ubus_client_init(5000); // 5 second timeout
    if (!ctx->ubus_client) {
        console_error(&csl, "Failed to initialize UBUS client");
        free(ctx);
        return NULL;
    }
    
    ctx->service_name = strdup(service_name);
    ctx->check_interval = check_interval;
    ctx->monitor_enabled = true;
    
    console_info(&csl, "Example service initialized successfully");
    return ctx;
}

/**
 * Start the example service with scheduler integration
 */
void start_example_service(Scheduler *sch, ExampleServiceContext *service_ctx) {
    if (!sch || !service_ctx) {
        console_error(&csl, "Invalid parameters for starting service");
        return;
    }
    
    console_info(&csl, "Starting example service with UBUS integration");
    
    // Run initial discovery and tests
    discover_services(service_ctx->ubus_client);
    multi_service_example(service_ctx->ubus_client);
    
    // Create task context for scheduler
    ExampleTaskContext *task_ctx = malloc(sizeof(ExampleTaskContext));
    if (!task_ctx) {
        console_error(&csl, "Failed to allocate task context");
        return;
    }
    
    task_ctx->service_ctx = service_ctx;
    task_ctx->scheduler = sch;
    
    // Schedule periodic monitoring
    schedule_task(sch, time(NULL) + service_ctx->check_interval,
                  example_monitoring_task, "example_monitoring", task_ctx);
    
    console_info(&csl, "Example service started with %d second monitoring interval", 
                service_ctx->check_interval);
}

/**
 * Cleanup the example service
 */
void cleanup_example_service(ExampleServiceContext *ctx) {
    if (!ctx) {
        return;
    }
    
    console_info(&csl, "Cleaning up example service");
    
    if (ctx->ubus_client) {
        ubus_client_cleanup(ctx->ubus_client);
    }
    
    if (ctx->service_name) {
        free(ctx->service_name);
    }
    
    free(ctx);
    console_info(&csl, "Example service cleanup complete");
}

/**
 * Enable/disable monitoring
 */
void set_monitoring_enabled(ExampleServiceContext *ctx, bool enabled) {
    if (ctx) {
        ctx->monitor_enabled = enabled;
        console_info(&csl, "Monitoring %s", enabled ? "enabled" : "disabled");
    }
}

/**
 * Example main function showing complete integration
 */
int example_main() {
    console_info(&csl, "Starting UBUS integration example");
    
    // Initialize scheduler (in real application, this would be shared)
    Scheduler *sch = init_scheduler();
    if (!sch) {
        console_error(&csl, "Failed to initialize scheduler");
        return 1;
    }
    
    // Initialize example service
    ExampleServiceContext *service = init_example_service("example-service", 30); // 30 second interval
    if (!service) {
        console_error(&csl, "Failed to initialize example service");
        return 1;
    }
    
    // Start the service
    start_example_service(sch, service);
    
    // In a real application, you would integrate this with your main event loop
    console_info(&csl, "Example service running... (in real app, this would be part of main loop)");
    
    // Cleanup (in real app, this would be in exit handlers)
    cleanup_example_service(service);
    // clean_scheduler(sch); // Uncomment if you have this function
    
    console_info(&csl, "UBUS integration example completed");
    return 0;
}