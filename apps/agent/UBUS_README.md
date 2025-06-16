# UBUS Integration for Wayru Agent

This document describes the UBUS server and client implementation for the Wayru Agent, enabling inter-process communication between the agent and other system services.

## Overview

The UBUS integration provides:
- **UBUS Server**: Exposes agent functionality via UBUS methods
- **UBUS Client**: Allows querying other UBUS services
- **Scheduler Integration**: Single-threaded operation with the agent's task scheduler
- **Extensible Design**: Easy to add new methods and services

## Architecture

### UBUS Server (`services/ubus_server.h/c`)
- Registers as `wayru-agent` service
- Exposes agent data through UBUS methods
- Integrates with the scheduler for periodic event processing
- Optimized for single-core devices

### UBUS Client (`services/ubus_client.h/c`)
- Provides API for calling other UBUS services
- Supports synchronous and asynchronous calls
- JSON conversion utilities
- Connection management

### Test Utility (`tools/ubus_test.c`)
- Command-line tool for testing UBUS functionality
- Examples of client usage
- Debugging and development aid

## Available UBUS Methods

The `wayru-agent` service exposes the following methods:

### `ping`
Simple ping/pong for connectivity testing.
```bash
ubus call wayru-agent ping
```
**Response:**
```json
{
  "response": "pong",
  "service": "wayru-agent",
  "timestamp": 1704067200
}
```

### `get_status`
Get overall agent status and service availability.
```bash
ubus call wayru-agent get_status
```
**Response:**
```json
{
  "service": "wayru-agent",
  "running": true,
  "access_token_available": true,
  "device_info_available": true,
  "registration_available": true,
  "token_valid": true
}
```

### `get_device_info`
Get comprehensive device information.
```bash
ubus call wayru-agent get_device_info
```
**Response:**
```json
{
  "device_id": "wayru_device_123",
  "mac": "aa:bb:cc:dd:ee:ff",
  "name": "Wayru Device",
  "brand": "Wayru",
  "model": "WR-100",
  "arch": "mips",
  "public_ip": "203.0.113.1",
  "os_name": "OpenWrt",
  "os_version": "22.03.2",
  "os_services_version": "1.0.0",
  "did_public_key": "base64_encoded_key"
}
```

### `get_access_token`
Get current access token information.
```bash
ubus call wayru-agent get_access_token
```
**Response:**
```json
{
  "token": "jwt_token_here",
  "issued_at": 1704067200,
  "expires_at": 1704153600,
  "valid": true
}
```

### `get_registration`
Get device registration information.
```bash
ubus call wayru-agent get_registration
```
**Response:**
```json
{
  "wayru_device_id": "device_123",
  "access_key": "access_key_here"
}
```

## Integration with Main Agent

The UBUS server is integrated into the main agent application:

```c
#include "services/ubus_server.h"

// In main() after scheduler initialization:
ubus_server_service(sch, access_token, device_info, registration);
```

The server:
1. Initializes UBUS connection
2. Registers the `wayru-agent` service
3. Schedules periodic UBUS event processing
4. Handles automatic reconnection on connection loss

## Using the UBUS Client

### Basic Usage

```c
#include "services/ubus_client.h"

// Initialize client
UbusClient *client = ubus_client_init(5000); // 5 second timeout

// Call a method
UbusResponse *response = ubus_client_call(client, "wayru-agent", "get_status", NULL);

if (response && response->success) {
    printf("Response: %s\n", response->json_response);
    
    // Extract specific values
    bool running = ubus_response_get_bool(response, "running", false);
    const char *service = ubus_response_get_string(response, "service");
}

// Cleanup
ubus_response_free(response);
ubus_client_cleanup(client);
```

### JSON Arguments

```c
// Call with JSON arguments
UbusResponse *response = ubus_client_call_json(client, "service", "method", 
                                              "{\"param1\": \"value1\", \"param2\": 42}");
```

### Asynchronous Calls

```c
void callback(UbusResponse *response, void *user_data) {
    printf("Async response: %s\n", response->json_response);
}

ubus_client_call_async(client, "service", "method", NULL, callback, NULL);
```

### Service Discovery

```c
// List all services
UbusResponse *services = ubus_client_list_services(client);

// Get methods for a service
UbusResponse *methods = ubus_client_get_service_methods(client, "wayru-agent");

// Ping a service
bool available = ubus_client_ping_service(client, "wayru-agent");
```

## Test Utility Usage

The `ubus_test` utility provides command-line access to UBUS functionality:

### Build the Test Utility

```bash
cd wayru-os-services/apps/agent/tools
make
```

### Basic Commands

```bash
# Test all wayru-agent methods
./bin/ubus_test -a

# Call specific method
./bin/ubus_test wayru-agent get_status

# List all services
./bin/ubus_test -l

# List methods for a service
./bin/ubus_test -m wayru-agent

# Ping a service
./bin/ubus_test -p wayru-agent

# Verbose output
./bin/ubus_test -v wayru-agent get_device_info

# Custom timeout
./bin/ubus_test -t 10000 wayru-agent get_access_token

# Call with JSON arguments
./bin/ubus_test -j '{"key": "value"}' service method
```

### Example Output

```bash
$ ./bin/ubus_test wayru-agent get_status
Calling wayru-agent.get_status...

SUCCESS:
{
  "service": "wayru-agent",
  "running": true,
  "access_token_available": true,
  "device_info_available": true,
  "registration_available": true,
  "token_valid": true
}
```

## Error Handling

The UBUS integration includes comprehensive error handling:

### Server Errors
- Connection loss detection and automatic reconnection
- Invalid method parameter handling
- Service unavailable responses
- Memory allocation failure handling

### Client Errors
- Service not found
- Method not found
- Timeout handling
- Connection errors
- Invalid JSON arguments

### Error Response Format
```json
{
  "error": "Service 'nonexistent' not found",
  "code": -2
}
```

## Performance Characteristics

### Single-Core Optimization
- Non-blocking UBUS event processing
- Integrated with scheduler to avoid blocking
- Minimal memory footprint
- Efficient JSON serialization

### Resource Usage
- ~1KB memory per active UBUS connection
- ~100μs per UBUS call processing
- Automatic cleanup of stale connections
- Configurable timeouts

## Extending the UBUS Interface

### Adding New Methods

1. **Define the method handler:**
```c
static int method_new_feature(struct ubus_context *ctx, struct ubus_object *obj,
                             struct ubus_request_data *req, const char *method,
                             struct blob_attr *msg) {
    struct blob_buf response;
    blob_buf_init(&response, 0);
    
    // Add your response data
    blobmsg_add_string(&response, "result", "success");
    
    int ret = ubus_send_reply(ctx, req, response.head);
    blob_buf_free(&response);
    return ret;
}
```

2. **Add to methods array:**
```c
static const struct ubus_method wayru_methods[] = {
    // ... existing methods
    UBUS_METHOD_NOARG("new_feature", method_new_feature),
};
```

3. **Update context if needed:**
```c
typedef struct {
    AccessToken *access_token;
    DeviceInfo *device_info;
    Registration *registration;
    NewService *new_service;  // Add new service
    void *reserved[7];        // Maintain reserved space
} UbusServerContext;
```

### Adding Service Context

The `UbusServerContext` includes reserved pointers for future extensions:

```c
// Add new service to context
server_context->reserved[0] = my_new_service;

// Access in method handler
MyService *service = (MyService *)server_context->reserved[0];
```

## Troubleshooting

### Common Issues

1. **Service not found**
   - Check if agent is running: `ps | grep wayru-os-services`
   - Verify UBUS is running: `ubus list`
   - Check system logs: `logread | grep wayru`

2. **Connection timeout**
   - Increase timeout: `ubus_client_init(10000)`
   - Check system load
   - Verify UBUS daemon is responsive

3. **Permission denied**
   - Check UBUS permissions
   - Verify user has access to UBUS socket
   - Run as appropriate user

4. **Method not found**
   - List available methods: `ubus list wayru-agent`
   - Check method name spelling
   - Verify agent version supports method

### Debug Commands

```bash
# List all UBUS services
ubus list

# Show wayru-agent methods
ubus list wayru-agent

# Call method directly
ubus call wayru-agent ping

# Monitor UBUS traffic
ubus monitor

# Check UBUS daemon status
/etc/init.d/ubus status
```

### Logging

Enable debug logging in the agent to see UBUS activity:

```c
static Console csl = {
    .topic = "ubus_server",
    .level = CONSOLE_DEBUG,  // Enable debug output
};
```

## Security Considerations

1. **Access Control**: UBUS methods should validate caller permissions
2. **Data Exposure**: Sensitive data (like access tokens) should be carefully exposed
3. **Rate Limiting**: Consider implementing rate limiting for resource-intensive methods
4. **Input Validation**: Always validate method parameters

## Future Enhancements

- **Method Authentication**: Add caller authentication for sensitive methods
- **Event Notifications**: Support for UBUS event broadcasting
- **Batch Operations**: Support for multiple method calls in one request
- **Configuration Methods**: Runtime configuration changes via UBUS
- **Statistics**: Performance and usage statistics via UBUS methods