# UBUS Quick Start Guide

Get up and running with Wayru Agent UBUS integration in 5 minutes.

## Prerequisites

Ensure UBUS is working on your system:
```bash
# Verify UBUS daemon is running
ubus list

# Check if wayru-agent is available (should show 5 methods)
ubus list wayru-agent
```

If `wayru-agent` is not listed, start the agent:
```bash
/etc/init.d/wayru-os-services start
```

## 2-Minute Test

Test basic functionality:
```bash
# Basic connectivity
ubus call wayru-agent ping

# Service health
ubus call wayru-agent get_status

# Device information
ubus call wayru-agent get_device_info
```

Expected response: JSON objects with service data.

## Quick Shell Examples

### Health Check Script
```bash
#!/bin/bash
if ubus call wayru-agent ping >/dev/null 2>&1; then
    echo "✓ Agent is healthy"
    
    # Get specific values
    DEVICE_ID=$(ubus call wayru-agent get_registration 2>/dev/null | jsonfilter -e '@.wayru_device_id')
    TOKEN_VALID=$(ubus call wayru-agent get_access_token 2>/dev/null | jsonfilter -e '@.valid')
    
    [ -n "$DEVICE_ID" ] && echo "  Device ID: $DEVICE_ID"
    [ "$TOKEN_VALID" = "true" ] && echo "  Token: Valid" || echo "  Token: Invalid"
else
    echo "✗ Agent is not responding"
fi
```

### One-Line Checks
```bash
# Agent health
ubus call wayru-agent ping && echo "Agent OK" || echo "Agent DOWN"

# Token status  
ubus call wayru-agent get_access_token | jsonfilter -e '@.valid' | grep -q true && echo "Token OK" || echo "Token INVALID"

# Service availability
ubus list | grep -q wayru-agent && echo "Service registered" || echo "Service NOT registered"
```

## Simple C Client

```c
#include "services/ubus_client.h"
#include <stdio.h>

int main() {
    UbusClient *client = ubus_client_init(5000);
    if (!client) {
        printf("Failed to initialize UBUS client\n");
        return 1;
    }

    // Check agent health
    UbusResponse *response = ubus_client_call(client, "wayru-agent", "get_status", NULL);
    
    if (response && response->success) {
        bool running = ubus_response_get_bool(response, "running", false);
        bool token_valid = ubus_response_get_bool(response, "token_valid", false);
        
        printf("Agent: %s, Token: %s\n", 
               running ? "Running" : "Stopped",
               token_valid ? "Valid" : "Invalid");
    } else {
        printf("Agent not responding\n");
    }

    ubus_response_free(response);
    ubus_client_cleanup(client);
    return 0;
}
```

## Test Utility

Build and use the test utility:
```bash
# Build
cd wayru-os-services/apps/agent/tools
make

# Test all methods
./bin/ubus_test -a

# Test specific functionality
./bin/ubus_test wayru-agent get_status
./bin/ubus_test -p wayru-agent  # Ping test
./bin/ubus_test -l              # List all services
```

## Quick Troubleshooting

**Problem**: Service not found
```bash
# Start the agent
/etc/init.d/wayru-os-services start

# Check agent status
/etc/init.d/wayru-os-services status
```

**Problem**: Permission denied
```bash
# Run as root
sudo ubus call wayru-agent ping
```

**Problem**: Timeout
```bash
# Use longer timeout
ubus -t 30 call wayru-agent get_device_info
```

**Problem**: Command not found
```bash
# Install ubus tools
opkg update && opkg install ubus
```

## Available Methods

| Method | Purpose |
|--------|---------|
| `ping` | Connectivity testing |
| `get_status` | Agent health and service availability |
| `get_device_info` | Complete device information |
| `get_access_token` | Current access token information |
| `get_registration` | Device registration details |

## Next Steps

1. **Full documentation**: See [ubus_readme.md](ubus_readme.md) for complete API reference
2. **Run comprehensive tests**: Use `scripts/test_ubus.sh` for validation
3. **Integration examples**: Check working examples in the codebase
4. **Monitor logs**: `logread -f | grep wayru` for troubleshooting

## Performance Tips

- **Reuse clients**: Don't create new UbusClient for each call
- **Check connectivity**: Use `ubus_client_ping_service()` before critical operations
- **Handle errors**: Always check response success and free responses
- **Use appropriate timeouts**: Short for non-critical, longer for important operations

The UBUS integration works reliably out of the box. Most operations should succeed on the first try with minimal configuration.