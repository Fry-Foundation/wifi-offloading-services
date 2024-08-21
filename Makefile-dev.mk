# Define development configuration
CONFIG_ENABLED = 1
CONFIG_MAIN_API = https://api.internal.wayru.tech
CONFIG_ACCOUNTING_ENABLED = 1
CONFIG_ACCOUNTING_INTERVAL = 300
CONFIG_ACCOUNTING_API = https://wifi.api.internal.wayru.tech
CONFIG_ACCESS_INTERVAL = 10800
CONFIG_DEVICE_STATUS_INTERVAL = 120
CONFIG_SETUP_INTERVAL = 120
CONFIG_LOG_LEVEL = 4
CONFIG_MONITORING_INTERVAL = 20

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

	cp certificates/ca.crt $(DIST_DATA_PATH)/ca.crt
	cp .env $(DIST_DATA_PATH)/.env
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
	--config-monitoring-interval "$(CONFIG_MONITORING_INTERVAL)"

# Clean the build
clean:
	rm -rf $(DIST_PATH)
