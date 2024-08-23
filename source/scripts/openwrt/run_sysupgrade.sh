#!/bin/sh

FIRMWARE_PATH="$1/firmware.bin"

if [ -z "$FIRMWARE_PATH" ]; then
    echo "-1"
    exit 1
fi

OUTPUT=$(sysupgrade -v -n "$FIRMWARE_PATH" 2>&1)

if echo "$OUTPUT" | grep -q "Commencing upgrade"; then
    echo "1"
else
    echo "-1"
fi
