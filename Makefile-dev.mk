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
	gcc -o $(DIST_PATH)/$(EXECUTABLE) $(SOURCE_PATH)/main.c $(SOURCE_PATH)/utils/script_runner.c $(SOURCE_PATH)/server.c $(SOURCE_PATH)/scheduler.c $(SOURCE_PATH)/requests.c $(SOURCE_PATH)/utils/base64.c $(SOURCE_PATH)/utils/generate_id.c -lpthread -lmicrohttpd -lcurl

# Copy scripts and data files
copy-scripts:
	mkdir -p $(DIST_SCRIPTS_PATH)
	cp $(SOURCE_PATH)/scripts/dev/* $(DIST_SCRIPTS_PATH)
	chmod +x $(DIST_SCRIPTS_PATH)/*

	mkdir -p $(DIST_DATA_PATH)

# Run the program
run:
	cd $(DIST_PATH) && ./$(EXECUTABLE) --dev

# Clean the build
clean:
	rm -rf $(DIST_PATH)
