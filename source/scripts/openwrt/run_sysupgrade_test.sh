#!/bin/sh

FIRMWARE_PATH="$1/firmware.bin"

if [ -z "$FIRMWARE_PATH" ] || [ ! -f "$FIRMWARE_PATH" ]; then
    exit 0 
fi

# Suppress all output from sysupgrade and capture its exit code
if sysupgrade --test "$FIRMWARE_PATH" >/dev/null 2>&1; then
    echo 1
    exit 1  # Success case
else
    echo 0
    exit 0  # Failure case
fi