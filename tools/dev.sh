#!/usr/bin/env bash

set -e

# Copy scripts and data files
echo "Copying scripts and data files..."
mkdir -p output/scripts
cp source/scripts/dev/* output/scripts/
chmod +x output/scripts/*

mkdir -p output/data
mkdir -p output/data/did-key

cp .env output/data/.env 2>/dev/null || true
cp VERSION output/VERSION

# Run the program
echo "Starting wayru-os-services in development mode..."
cd output && ./wayru-os-services --dev \
    --config-enabled "1" \
    --config-main-api "http://api.internal.wayru.tech" \
    --config-accounting-api "https://wifi.api.internal.wayru.tech" \
    --config-devices-api "http://devices.internal.wayru.tech" \
    --config-access-interval "10800" \
    --config-device-status-interval "120" \
    --config-console-log-level "4" \
    --config-monitoring-enabled "1" \
    --config-monitoring-interval "900" \
    --config-monitoring-minimum-interval "300" \
    --config-monitoring-maximum-interval "900" \
    --config-speed-test-enabled "1" \
    --config-speed-test-interval "10800" \
    --config-speed-test-minimum-interval "10800" \
    --config-speed-test-maximum-interval "21600" \
    --config-speed-test-latency-attempts "4" \
    --config-device-context-interval "900" \
    --config-mqtt-broker-url "broker.internal.wayru.tech" \
    --config-mqtt-keepalive "60" \
    --config-mqtt-task-interval "30" \
    --config-reboot-enabled "1" \
    --config-reboot-interval "88200" \
    --config-firmware-update-enabled "1" \
    --config-firmware-update-interval "86400" \
    --config-use-n-sysupgrade "0" \
    --config-package-update-enabled "1" \
    --config-package-update-interval "20000" \
    --config-diagnostic-interval "120" \
    --config-external-connectivity-host "google.com" \
    --config-nds-interval "60" \
    --config-time-sync-server "ptbtime1.ptb.de" \
    --config-time-sync-interval "3600" 