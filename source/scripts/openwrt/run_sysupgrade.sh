#!/bin/sh

FIRMWARE_PATH="$1/firmware.bin"
USE_N_OPTION="$2"

if [ -z "$FIRMWARE_PATH" ]; then
    echo "-1"
    exit 1
fi

OUTPUT=$(sysupgrade -v $USE_N_OPTION "$FIRMWARE_PATH" 2>&1)

if echo "$OUTPUT" | grep -q "Commencing upgrade"; then
    echo "1"
else
    echo "-1"
fi
