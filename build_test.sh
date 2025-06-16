#!/bin/bash

# Simple build script for scheduler test
echo "Building scheduler test..."

# Create test data directory
mkdir -p test_data

# Compile the test
gcc -o test_scheduler test_scheduler.c \
    lib/core/uloop_scheduler.c \
    lib/core/console.c \
    apps/agent/services/access_token.c \
    apps/agent/services/time_sync.c \
    -I. \
    -I./lib \
    -I./apps/agent \
    -lubox \
    -ljson-c \
    -lcrypto \
    -lssl \
    -lcurl

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Running test..."
    ./test_scheduler
else
    echo "Build failed!"
    exit 1
fi
