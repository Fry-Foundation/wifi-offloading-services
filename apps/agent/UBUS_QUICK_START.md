# UBUS Quick Start Guide

This guide helps developers quickly get started with the Wayru Agent UBUS integration.

## 5-Minute Setup

### 1. Check if UBUS is Working

```bash
# Verify UBUS daemon is running
ubus list

# Check if wayru-agent is available
ubus list wayru-agent

# Test basic connectivity
ubus call wayru-agent ping
```

### 2. Basic Usage Examples

#### Query Agent Status
```bash
ubus call wayru-agent get_status
```

#### Get Device Information
```bash
ubus call wayru-agent get_device_info
```

#### Check Access Token
```bash
ubus call wayru-agent get_access_token
```

### 3. Using the Test Utility

```bash
# Build and run comprehensive tests
cd wayru-os-services/apps/agent/tools
make
./bin/ubus_test -a
```

## Programming with UBUS Client

### Simple C Program

```c
#include "services/ubus_client.h"
#include <stdio.h>

int main() {
    // Initialize client
    UbusClient *client = ubus_client_init(5000);
    if (!client) {
        printf("Failed to initialize UBUS client\n");
        return 1;
    }

    // Call wayru-agent method
    UbusResponse *response = ubus_client_call(client, "wayru-agent", "get_status", NULL);
    
    if (response && response->success) {
        printf("Agent status: %s\n", response->json_response);
        
        // Extract specific values
        bool running = ubus_response_get_bool(response, "running", false);
        printf("Agent running: %s\n", running ? "yes" : "no");
    } else {
        printf("Failed to get status\n");
    }

    // Cleanup
    if (response) ubus_response_free(response);
    ubus_client_cleanup(client);
    return 0;
}
```

### Service Monitoring Example

```c
#include "services/ubus_client.h"

void monitor_agent() {
    UbusClient *client = ubus_client_init(5000);
    
    while (1) {
        // Check if service is available
        if (ubus_client_ping_service(client, "wayru-agent")) {
            printf("Agent is responsive\n");
        } else {
            printf("Agent is not responding\n");
        }
        
        sleep(30); // Check every 30 seconds
    }
    
    ubus_client_cleanup(client);
}
```

## Shell Scripting

### Basic Script
```bash
#!/bin/bash

# Check if agent is running
if ubus call wayru-agent ping >/dev/null 2>&1; then
    echo "Agent is running"
    
    # Get device ID
    DEVICE_ID=$(ubus call wayru-agent get_registration | jsonfilter -e '@.wayru_device_id')
    echo "Device ID: $DEVICE_ID"
    
    # Check token validity
    TOKEN_VALID=$(ubus call wayru-agent get_access_token | jsonfilter -e '@.valid')
    echo "Token valid: $TOKEN_VALID"
else
    echo "Agent is not responding"
    exit 1
fi
```

### Service Health Check
```bash
#!/bin/bash

check_service_health() {
    local service=$1
    
    if ubus call "$service" ping >/dev/null 2>&1; then
        echo "✓ $service is healthy"
        return 0
    else
        echo "✗ $service is not responding"
        return 1
    fi
}

# Check multiple services
check_service_health "wayru-agent"
check_service_health "system"
check_service_health "network"
```

## Adding New Methods to wayru-agent

### 1. Define Method Handler

In `services/ubus_server.c`:

```c
static int method_get_network_stats(struct ubus_context *ctx, struct ubus_object *obj,
                                   struct ubus_request_data *req, const char *method,
                                   struct blob_attr *msg) {
    struct blob_buf response;
    blob_buf_init(&response, 0);
    
    // Add your data
    blobmsg_add_string(&response, "interface", "eth0");
    blobmsg_add_u32(&response, "rx_bytes", 1024000);
    blobmsg_add_u32(&response, "tx_bytes", 512000);
    
    int ret = ubus_send_reply(ctx, req, response.head);
    blob_buf_free(&response);
    return ret;
}
```

### 2. Add to Methods Array

```c
static const struct ubus_method wayru_methods[] = {
    // ... existing methods
    UBUS_METHOD_NOARG("get_network_stats", method_get_network_stats),
};
```

### 3. Test New Method

```bash
ubus call wayru-agent get_network_stats
```

## Creating Your Own UBUS Service

### 1. Basic Service Template

```c
#include <libubus.h>
#include <libubox/blobmsg.h>

static struct ubus_context *ctx;
static struct ubus_object my_object;

static int my_method_hello(struct ubus_context *ctx, struct ubus_object *obj,
                          struct ubus_request_data *req, const char *method,
                          struct blob_attr *msg) {
    struct blob_buf b;
    blob_buf_init(&b, 0);
    blobmsg_add_string(&b, "message", "Hello from my service!");
    ubus_send_reply(ctx, req, b.head);
    blob_buf_free(&b);
    return 0;
}

static const struct ubus_method my_methods[] = {
    UBUS_METHOD_NOARG("hello", my_method_hello),
};

static struct ubus_object_type my_object_type =
    UBUS_OBJECT_TYPE("my-service", my_methods);

static struct ubus_object my_object = {
    .name = "my-service",
    .type = &my_object_type,
    .methods = my_methods,
    .n_methods = ARRAY_SIZE(my_methods),
};

int main() {
    uloop_init();
    
    ctx = ubus_connect(NULL);
    if (!ctx) {
        fprintf(stderr, "Failed to connect to ubus\n");
        return -1;
    }
    
    ubus_add_uloop(ctx);
    ubus_add_object(ctx, &my_object);
    
    uloop_run();
    
    ubus_free(ctx);
    uloop_done();
    return 0;
}
```

### 2. Build and Test

```bash
# Compile
gcc -o my-service my-service.c -lubus -lubox -lblobmsg_json

# Run
./my-service &

# Test
ubus call my-service hello
```

## Using UBUS Client in Other Services

### Integration with Existing Service

```c
#include "services/ubus_client.h"

typedef struct {
    UbusClient *ubus_client;
    // ... other service data
} MyServiceContext;

void my_service_init(MyServiceContext *ctx) {
    // Initialize UBUS client
    ctx->ubus_client = ubus_client_init(5000);
    if (!ctx->ubus_client) {
        fprintf(stderr, "Failed to init UBUS client\n");
        return;
    }
    
    // Your service initialization
}

void my_service_task(MyServiceContext *ctx) {
    // Check if we need agent data
    UbusResponse *response = ubus_client_call(ctx->ubus_client, 
                                             "wayru-agent", "get_status", NULL);
    
    if (response && response->success) {
        bool agent_running = ubus_response_get_bool(response, "running", false);
        if (agent_running) {
            // Agent is healthy, proceed with operations
            printf("Agent is running, continuing operations\n");
        }
    }
    
    if (response) ubus_response_free(response);
}

void my_service_cleanup(MyServiceContext *ctx) {
    if (ctx->ubus_client) {
        ubus_client_cleanup(ctx->ubus_client);
    }
}
```

## Common Use Cases

### 1. Service Health Monitoring

```c
bool check_agent_health(UbusClient *client) {
    UbusResponse *response = ubus_client_call(client, "wayru-agent", "get_status", NULL);
    
    if (!response || !response->success) {
        return false;
    }
    
    bool running = ubus_response_get_bool(response, "running", false);
    bool token_available = ubus_response_get_bool(response, "access_token_available", false);
    
    ubus_response_free(response);
    return running && token_available;
}
```

### 2. Device Information Retrieval

```c
char* get_device_mac(UbusClient *client) {
    UbusResponse *response = ubus_client_call(client, "wayru-agent", "get_device_info", NULL);
    
    if (!response || !response->success) {
        return NULL;
    }
    
    const char *mac = ubus_response_get_string(response, "mac");
    char *result = mac ? strdup(mac) : NULL;
    
    ubus_response_free(response);
    return result;
}
```

### 3. Access Token Validation

```c
bool is_agent_token_valid(UbusClient *client) {
    UbusResponse *response = ubus_client_call(client, "wayru-agent", "get_access_token", NULL);
    
    if (!response || !response->success) {
        return false;
    }
    
    bool valid = ubus_response_get_bool(response, "valid", false);
    ubus_response_free(response);
    return valid;
}
```

## Troubleshooting

### Quick Diagnostics

```bash
# 1. Check UBUS daemon
/etc/init.d/ubus status

# 2. List all services
ubus list

# 3. Check agent specifically
ubus list wayru-agent

# 4. Test connectivity
ubus call wayru-agent ping

# 5. Check agent process
pgrep -f wayru-os-services

# 6. Monitor logs
logread -f | grep wayru
```

### Common Issues

**Problem**: `ubus: not found`
**Solution**: Install UBUS: `opkg install ubus`

**Problem**: `Service 'wayru-agent' not found`
**Solution**: Start the agent: `/etc/init.d/wayru-os-services start`

**Problem**: `Permission denied`
**Solution**: Check user permissions or run as root

**Problem**: `Connection timeout`
**Solution**: Increase timeout or check system load

## Performance Tips

### 1. Reuse UBUS Clients
```c
// Good: Reuse client
UbusClient *client = ubus_client_init(5000);
for (int i = 0; i < 100; i++) {
    UbusResponse *response = ubus_client_call(client, "wayru-agent", "ping", NULL);
    // ... process response
    ubus_response_free(response);
}
ubus_client_cleanup(client);

// Bad: Create new client each time
for (int i = 0; i < 100; i++) {
    UbusClient *client = ubus_client_init(5000);
    UbusResponse *response = ubus_client_call(client, "wayru-agent", "ping", NULL);
    // ... process response
    ubus_response_free(response);
    ubus_client_cleanup(client);
}
```

### 2. Check Connection Before Calls
```c
if (!ubus_client_is_connected(client)) {
    // Reconnect logic
    ubus_client_cleanup(client);
    client = ubus_client_init(5000);
}
```

### 3. Use Appropriate Timeouts
```c
// For critical operations: longer timeout
UbusClient *client = ubus_client_init(10000);

// For non-critical: shorter timeout
UbusClient *client = ubus_client_init(1000);
```

## Next Steps

1. **Try the examples**: Copy-paste code above and test
2. **Read the full documentation**: See `UBUS_README.md`
3. **Run the test suite**: Use `tools/test_ubus.sh`
4. **Explore integration examples**: Check `examples/` directory
5. **Build your own service**: Use the templates provided

## Getting Help

- Check logs: `logread | grep wayru`
- Run diagnostics: `tools/test_ubus.sh --quick`
- Test with utility: `tools/bin/ubus_test -a`
- Monitor traffic: `ubus monitor`

The UBUS integration is designed to be simple and reliable. Most operations should work out of the box with minimal configuration.