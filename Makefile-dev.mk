# Define development configuration
CONFIG_ENABLED = 1
CONFIG_MAIN_API = https://api.prod-2.wayru.tech
CONFIG_ACCOUNTING_ENABLED = 1
CONFIG_ACCOUNTING_INTERVAL = 300
CONFIG_ACCOUNTING_API = https://wifi.api.wayru.tech
CONFIG_ACCESS_INTERVAL = 10800
CONFIG_DEVICE_STATUS_INTERVAL = 120
CONFIG_SETUP_INTERVAL = 120
CONFIG_LOG_LEVEL = 3
CONFIG_MONITORING_ENABLED = 1
CONFIG_MONITORING_INTERVAL = 900
CONFIG_MONITORING_MINIMUM_INTERVAL = 300
CONFIG_MONITORING_MAXIMUM_INTERVAL = 900
CONFIG_SPEED_TEST_ENABLED = 1
CONFIG_SPEED_TEST_API = https://speedtest.api.internal.wayru.tech
CONFIG_SPEED_TEST_API_KEY = nwde9UEXUDRcUp6hTuasrpmTcTP8Gxn2
CONFIG_SPEED_TEST_INTERVAL = 10800
CONFIG_SPEED_TEST_MINIMUM_INTERVAL = 1#10800
CONFIG_SPEED_TEST_MAXIMUM_INTERVAL = 50#21600
CONFIG_SPEED_TEST_BACKHAUL_ATTEMPTS = 3
CONFIG_SPEED_TEST_LATENCY_ATTEMPTS = 4
CONFIG_SPEED_TEST_UPLOAD_LIMIT = 800
CONFIG_DEVICE_CONTEXT_INTERVAL = 900
CONFIG_SITE_CLIENTS_INTERVAL = 10
CONFIG_MQTT_BROKER_URL = mqtt.wayru.tech
CONFIG_REBOOT_ENABLED = 1
CONFIG_REBOOT_INTERVAL = 88200
CONFIG_FIRMWARE_UPDATE_ENABLED = 1
CONFIG_FIRMWARE_UPDATE_INTERVAL = 86400
CONFIG_USE_N_SYSUPGRADE = 0
CONFIG_DIAGNOSTIC_INTERVAL = 600

# Define paths
SOURCE_PATH := source
DIST_PATH := dist
DIST_SCRIPTS_PATH := $(DIST_PATH)/scripts
DIST_DATA_PATH := $(DIST_PATH)/data
DIST_DID_KEY_PATH := $(DIST_DATA_PATH)/did-key
DIST_DATA_PATH := $(DIST_PATH)/tmp

# Define executable
EXECUTABLE := wayru-os-services

# Find all .c files recursively and list them as sources
SOURCES = $(shell find $(SOURCE_PATH) -name '*.c')
$(info SOURCES: $(SOURCES))

# Convert the source files to object files in the dist directory
OBJECTS = $(patsubst $(SOURCE_PATH)/%.c, $(DIST_PATH)/%.o, $(SOURCES))
$(info OBJECTS: $(OBJECTS))

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -I$(SOURCE_PATH)
# CFLAGS = -g -Wall -Wextra -std=gnu11 -I$(SOURCE_PATH)

# Libraries
LIBS = -lcurl -ljson-c -lssl -lcrypto -lmosquitto

.PHONY: all compile-only compile run copy-scripts clean
all: clean compile copy-scripts run
compile-only: clean compile copy-scripts

# Compile the program
compile: $(DIST_PATH)/$(EXECUTABLE)

$(DIST_PATH)/$(EXECUTABLE): $(OBJECTS)
	mkdir -p $(DIST_PATH)
	$(CC) -o $@ $^ $(LIBS)

$(DIST_PATH)/%.o: $(SOURCE_PATH)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# Copy scripts and data files
copy-scripts:
	mkdir -p $(DIST_SCRIPTS_PATH)
	cp $(SOURCE_PATH)/scripts/dev/* $(DIST_SCRIPTS_PATH)
	chmod +x $(DIST_SCRIPTS_PATH)/*

	mkdir -p $(DIST_DATA_PATH)
	mkdir -p $(DIST_DID_KEY_PATH)

	cp .env $(DIST_DATA_PATH)/.env || true
	cp VERSION $(DIST_PATH)/VERSION

# Run the program
run:
	cd "$(DIST_PATH)" && ./$(EXECUTABLE) --dev \
	--config-enabled "$(CONFIG_ENABLED)" \
	--config-main-api "$(CONFIG_MAIN_API)" \
	--config-accounting-enabled "$(CONFIG_ACCOUNTING_ENABLED)" \
	--config-accounting-interval "$(CONFIG_ACCOUNTING_INTERVAL)" \
	--config-accounting-api "$(CONFIG_ACCOUNTING_API)" \
	--config-access-interval "$(CONFIG_ACCESS_INTERVAL)" \
	--config-device-status-interval "$(CONFIG_DEVICE_STATUS_INTERVAL)" \
	--config-setup-interval "$(CONFIG_SETUP_INTERVAL)" \
	--config-console-log-level "$(CONFIG_LOG_LEVEL)" \
	--config-monitoring-enabled "$(CONFIG_MONITORING_ENABLED)" \
	--config-monitoring-interval "$(CONFIG_MONITORING_INTERVAL)" \
	--config-monitoring-minimum-interval "$(CONFIG_MONITORING_MINIMUM_INTERVAL)" \
	--config-monitoring-maximum-interval "$(CONFIG_MONITORING_MAXIMUM_INTERVAL)" \
	--config-speed-test-enabled "$(CONFIG_SPEED_TEST_ENABLED)" \
	--config-speed-test-api "$(CONFIG_SPEED_TEST_API)" \
	--config-speed-test-api-key "$(CONFIG_SPEED_TEST_API_KEY)" \
	--config-speed-test-interval "$(CONFIG_SPEED_TEST_INTERVAL)" \
	--config-speed-test-minimum-interval "$(CONFIG_SPEED_TEST_MINIMUM_INTERVAL)" \
	--config-speed-test-maximum-interval "$(CONFIG_SPEED_TEST_MAXIMUM_INTERVAL)" \
	--config-speed-test-backhaul-attempts "$(CONFIG_SPEED_TEST_BACKHAUL_ATTEMPTS)" \
	--config-speed-test-latency-attempts "$(CONFIG_SPEED_TEST_LATENCY_ATTEMPTS)" \
	--config-speed-test-upload-limit "$(CONFIG_SPEED_TEST_UPLOAD_LIMIT)" \
	--config-device-context-interval "$(CONFIG_DEVICE_CONTEXT_INTERVAL)" \
	--config-site-clients-interval "$(CONFIG_SITE_CLIENTS_INTERVAL)" \
	--config-mqtt-broker-url "$(CONFIG_MQTT_BROKER_URL)" \
	--config-reboot-enabled "$(CONFIG_REBOOT_ENABLED)" \
	--config-reboot-interval "$(CONFIG_REBOOT_INTERVAL)" \
	--config-firmware-update-enabled "$(CONFIG_FIRMWARE_UPDATE_ENABLED)" \
	--config-firmware-update-interval "$(CONFIG_FIRMWARE_UPDATE_INTERVAL)" \
	--config-use-n-sysupgrade"$(CONFIG_USE_N_SYSUPGRADE)" \
	--config-diagnostic-interval"$(CONFIG_DIAGNOSTIC_INTERVAL)" \

# Clean the build
clean:
	rm -rf $(DIST_PATH)
