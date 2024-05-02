# Define development configuration
CONFIG_ENABLED = 1
CONFIG_MAIN_API = https://api.internal.wayru.tech
CONFIG_ACCOUNTING_ENABLED = 1
CONFIG_ACCOUNTING_INTERVAL = 300
CONFIG_ACCOUNTING_API = https://wifi.api.internal.wayru.tech
CONFIG_ACCESS_TASK_INTERVAL = 120

# Define paths
SOURCE_PATH := source
DIST_PATH := dist
DIST_SCRIPTS_PATH := $(DIST_PATH)/scripts
DIST_DATA_PATH := $(DIST_PATH)/data

# Define executable
EXECUTABLE := wayru-os-services

.PHONY: all compile run copy-scripts clean
all: clean compile copy-scripts run

# Compile the program
compile:
	mkdir -p $(DIST_PATH)
	gcc -o $(DIST_PATH)/$(EXECUTABLE) \
	$(SOURCE_PATH)/main.c \
	$(SOURCE_PATH)/services/init.c \
	$(SOURCE_PATH)/services/scheduler.c \
	$(SOURCE_PATH)/services/access.c \
	$(SOURCE_PATH)/services/setup.c \
	$(SOURCE_PATH)/services/accounting.c \
	$(SOURCE_PATH)/services/end_report.c \
	$(SOURCE_PATH)/services/peaq_id.c \
	$(SOURCE_PATH)/store/config.c \
	$(SOURCE_PATH)/store/state.c \
	$(SOURCE_PATH)/utils/requests.c \
	$(SOURCE_PATH)/utils/script_runner.c \
	$(SOURCE_PATH)/utils/console.c \
	-lcurl -ljson-c -lssl -lcrypto

# Copy scripts and data files
copy-scripts:
	mkdir -p $(DIST_SCRIPTS_PATH)
	cp $(SOURCE_PATH)/scripts/dev/* $(DIST_SCRIPTS_PATH)
	chmod +x $(DIST_SCRIPTS_PATH)/*

	mkdir -p $(DIST_DATA_PATH)

	cp VERSION $(DIST_PATH)/VERSION

# Run the program
run:
	cd $(DIST_PATH) && ./$(EXECUTABLE) --dev

# Clean the build
clean:
	rm -rf $(DIST_PATH)
