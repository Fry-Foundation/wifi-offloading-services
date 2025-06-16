CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O2
PKG_CONFIG = pkg-config

# libubox package name
LIBUBOX_PKG = libubox

# Check if libubox is available
LIBUBOX_AVAILABLE := $(shell $(PKG_CONFIG) --exists $(LIBUBOX_PKG) && echo yes || echo no)

ifeq ($(LIBUBOX_AVAILABLE),yes)
    LIBUBOX_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(LIBUBOX_PKG))
    LIBUBOX_LIBS := $(shell $(PKG_CONFIG) --libs $(LIBUBOX_PKG))
else
    # Fallback for systems without pkg-config or libubox package
    LIBUBOX_CFLAGS = -I/usr/include/libubox
    LIBUBOX_LIBS = -lubox
endif

# Include directories
INCLUDES = -I../lib -I../lib/core

# Final compilation flags
CFLAGS += $(INCLUDES) $(LIBUBOX_CFLAGS)
LDFLAGS = $(LIBUBOX_LIBS)

# Source files
SCHEDULER_SRCS = ../lib/core/uloop_scheduler.c ../lib/core/console.c
EXAMPLE_SRCS = scheduler_example.c

# Object files
SCHEDULER_OBJS = $(SCHEDULER_SRCS:.c=.o)
EXAMPLE_OBJS = $(EXAMPLE_SRCS:.c=.o)

# Target executable
TARGET = scheduler_example

.PHONY: all clean check-deps

all: check-deps $(TARGET)

check-deps:
	@echo "Checking dependencies..."
	@$(PKG_CONFIG) --exists $(LIBUBOX_PKG) || (echo "Warning: libubox not found via pkg-config, using fallback flags" && true)
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"

$(TARGET): $(SCHEDULER_OBJS) $(EXAMPLE_OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning up..."
	rm -f $(TARGET) $(SCHEDULER_OBJS) $(EXAMPLE_OBJS)

# Help target
help:
	@echo "Available targets:"
	@echo "  all        - Build the scheduler example (default)"
	@echo "  clean      - Remove compiled files"
	@echo "  check-deps - Check for required dependencies"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Requirements:"
	@echo "  - libubox development headers"
	@echo "  - GCC compiler"
	@echo ""
	@echo "On OpenWrt/LEDE:"
	@echo "  opkg install libubox-dev"
	@echo ""
	@echo "On Ubuntu/Debian:"
	@echo "  apt-get install libubox-dev"

# Install target (optional)
install: $(TARGET)
	@echo "Installing $(TARGET) to /usr/local/bin..."
	install -m 755 $(TARGET) /usr/local/bin/

.SUFFIXES: .c .o
