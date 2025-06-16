# Collector Development Quick Start Guide

This guide helps you quickly set up and test the Wayru OS Collector in a local development environment.

## Quick Start

### 1. Build and Run Collector

```bash
# From project root
just dev collector

# Or manually:
just cmake
mkdir -p dev/collector
cp build/collector dev/collector
cd dev/collector && ./collector --dev
```

### 2. Start Mock Backend (Optional)

In a separate terminal:

```bash
cd dev/collector/scripts
python3 mock-backend.py --verbose
```

The mock backend will start on `http://localhost:8080` and accept log submissions.

### 3. Generate Test Logs

In another terminal:

```bash
cd dev/collector/scripts

# Generate 10 normal test logs
./test-logs.sh 10 1 normal

# Quick burst test
./test-logs.sh 50 0 burst

# Test filtering functionality
./test-logs.sh 10 1 filtered

# Test batch processing
./test-logs.sh 50 0 batch
```

## Development Files

- **`collector.conf`** - Development configuration settings
- **`test-logs.sh`** - Script to generate test syslog messages
- **`mock-backend.py`** - Local HTTP server for testing
- **`README.md`** - This guide

## What You'll See

### Collector Output (--dev mode)
```
[collector] Collector service started in development mode (single-core optimized)
[collector] Detected 4 CPU core(s) - using single-threaded event loop
[ubus] UBUS initialization complete (single-core mode)
[collect] Single-core collection system initialized (max_queue_size=500, max_batch_size=50)
[collector] Status: queue_size=12, dropped=0, ubus_connected=yes
[collect] Starting batch: reached max size (50)
[collect] Successfully sent batch of 50 logs
```

### Mock Backend Output
```
[14:30:25] Received batch: 50 logs from collector v1.0.0-single-core
  [1] sshd.auth.info: Connection established from 192.168.1.100
  [2] nginx.daemon.warning: Warning: disk space low
  ...
```

## Testing Scenarios

### 1. Normal Operation
```bash
./test-logs.sh 20 2 normal
```
- Generates 20 logs with 2-second intervals
- Tests normal batching behavior
- Verifies queue management

### 2. Burst Testing
```bash
./test-logs.sh 100 0 burst
```
- Rapid log generation
- Tests queue handling under load
- Verifies urgent batch processing

### 3. Filter Testing
```bash
./test-logs.sh 10 1 filtered
```
- Mixed valid and filtered messages
- Tests collector filtering logic
- Shows which logs are processed/dropped

### 4. Batch Testing
```bash
./test-logs.sh 50 0 batch
```
- Exactly batch-size number of logs
- Tests batch size triggering
- Tests timeout-based batching

### 5. Stress Testing
```bash
./test-logs.sh 100 0 stress
```
- High volume log generation
- Tests system under stress
- Monitors queue overflow protection

## Configuration Options

Edit `collector.conf` to modify behavior:

```bash
# Backend settings
BACKEND_URL="http://localhost:8080/v1/logs"
BATCH_SIZE=10                    # Smaller batches for testing
BATCH_TIMEOUT_MS=5000           # 5-second timeout

# Development settings
DEV_MODE=true
VERBOSE_LOGGING=true
STATUS_INTERVAL=10              # Status every 10 seconds

# Testing features
USE_MOCK_BACKEND=true
SIMULATE_BACKEND_FAILURES=false
```

## Mock Backend Options

```bash
# Basic server
python3 mock-backend.py

# Verbose logging
python3 mock-backend.py --verbose

# Simulate failures (10% failure rate)
python3 mock-backend.py --simulate-failures --failure-rate 0.1

# Simulate network delays
python3 mock-backend.py --simulate-delay

# Custom host/port
python3 mock-backend.py --host 0.0.0.0 --port 9090
```

## Monitoring

### Real-time Status
The collector logs status every 30 seconds in dev mode:
```
Status: queue_size=15, dropped=0, ubus_connected=yes
```

### Backend Statistics
Check mock backend stats:
```bash
curl http://localhost:8080/stats
```

### Health Check
```bash
curl http://localhost:8080/health
```

## Troubleshooting

### Collector Not Receiving Logs
1. Check if `logd` service is running
2. Verify UBUS connection: `ubus list | grep log`
3. Check collector logs for UBUS connection errors

### No Logs Reaching Backend
1. Verify mock backend is running on correct port
2. Check collector HTTP state machine logs
3. Test backend manually with curl

### High Memory Usage
1. Check queue size in status messages
2. Verify logs are being processed and sent
3. Check for backend connectivity issues

### Queue Overflows
1. Reduce log generation rate
2. Increase batch processing frequency
3. Check backend response times

## Advanced Testing

### Manual HTTP Testing
```bash
curl -X POST http://localhost:8080/v1/logs \
  -H "Content-Type: application/json" \
  -d '{
    "logs": [
      {
        "program": "test",
        "message": "Manual test message",
        "facility": "user",
        "priority": "info",
        "timestamp": 1640995200
      }
    ],
    "count": 1,
    "collector_version": "manual-test"
  }'
```

### UBUS Testing
```bash
# List UBUS objects
ubus list

# Subscribe to log events manually
ubus subscribe log

# Send test log
logger -t "test-app" -p user.info "Test message"
```

### Performance Testing
Monitor collector performance:
```bash
# CPU usage
top -p $(pgrep collector)

# Memory usage
cat /proc/$(pgrep collector)/status | grep VmRSS

# Network connections
netstat -tp | grep collector
```

## Development Tips

1. **Use smaller batch sizes** for faster testing feedback
2. **Enable verbose logging** to see internal operations
3. **Use mock backend** to avoid external dependencies
4. **Test different log volumes** to verify scalability
5. **Monitor queue statistics** to understand behavior
6. **Test network failures** by stopping mock backend

## Integration Testing

To test with actual system logs:
```bash
# Generate real system activity
sudo systemctl restart networking
ssh localhost  # Generate auth logs
wget http://example.com  # Generate network logs
```

The collector will automatically pick up these real system logs along with your test messages.