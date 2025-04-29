# Default recipe
default:
    @just --list

# Development environment setup
dev:
    @make -f Makefile-dev.mk

# Generate compilation database
beargen:
    cd tools && ./beargen.sh

# Format code
format:
    cd tools && ./format.sh

# Compile for specific architecture
compile arch:
    cd tools/compile && go run compile.go {{arch}} 