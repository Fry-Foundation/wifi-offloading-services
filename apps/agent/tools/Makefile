CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE
LDFLAGS = -lubus -lubox -lblobmsg_json -ljson-c

# Directories
CORE_DIR = ../../../lib/core
SERVICES_DIR = ../services
BUILD_DIR = build
TARGET_DIR = bin

# Include paths
INCLUDES = -I../../../lib -I..

# Source files
SOURCES = ubus_test.c \
          $(SERVICES_DIR)/ubus_client.c \
          $(CORE_DIR)/console.c

# Object files
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

# Target executable
TARGET = $(TARGET_DIR)/ubus_test

# Default target
all: $(TARGET)

# Create directories
$(BUILD_DIR) $(TARGET_DIR):
	mkdir -p $@

# Create nested build directories
$(BUILD_DIR)/$(SERVICES_DIR) $(BUILD_DIR)/$(CORE_DIR): | $(BUILD_DIR)
	mkdir -p $@

# Compile object files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR) $(BUILD_DIR)/$(SERVICES_DIR) $(BUILD_DIR)/$(CORE_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Link executable
$(TARGET): $(OBJECTS) | $(TARGET_DIR)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(TARGET_DIR)

# Install target (optional)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

# Release build
release: CFLAGS += -O2 -DNDEBUG
release: $(TARGET)

# Test target
test: $(TARGET)
	@echo "Running UBUS test utility..."
	@echo "Testing wayru-agent service:"
	./$(TARGET) -a

# Check dependencies
check-deps:
	@echo "Checking required dependencies..."
	@pkg-config --exists libubus && echo "libubus: OK" || echo "libubus: MISSING"
	@pkg-config --exists libubox && echo "libubox: OK" || echo "libubox: MISSING"
	@pkg-config --exists json-c && echo "json-c: OK" || echo "json-c: MISSING"

# Help target
help:
	@echo "Available targets:"
	@echo "  all      - Build the UBUS test utility (default)"
	@echo "  clean    - Remove build files"
	@echo "  debug    - Build with debug symbols"
	@echo "  release  - Build optimized release version"
	@echo "  install  - Install to /usr/local/bin/"
	@echo "  test     - Run basic tests"
	@echo "  check-deps - Check if required dependencies are available"
	@echo "  help     - Show this help message"

.PHONY: all clean install debug release test check-deps help
