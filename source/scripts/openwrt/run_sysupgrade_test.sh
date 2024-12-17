#!/bin/sh

FIRMWARE_PATH="$1/firmware.bin"

if [ -z "$FIRMWARE_PATH" ] || [ ! -f "$FIRMWARE_PATH" ]; then
    exit 0 
fi

sysupgrade --test "$FIRMWARE_PATH"

if [ $? -eq 0 ]; then
    echo 1
    exit 1  
else
    echo 0
    exit 0  
fi
