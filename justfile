# Default recipe - shows available commands
default:
    @just --list

# Development environment setup for specific app
# Available apps: agent, health, collector
dev app="agent":
    just cmake
    @if [ ! -f "build/{{app}}" ]; then echo "Error: App '{{app}}' not found in build directory."; exit 1; fi
    mkdir -p dev/{{app}}
    cp build/{{app}} dev/{{app}}
    bash tools/dev.sh {{app}}

# Generate compilation database (compile_commands.json)
compdb:
    bash tools/compdb.sh

# Format code
format:
    cd tools/format && ./format.sh

# Compile for specific architecture
compile arch:
    cd tools/compile && go run compile.go {{arch}}

# Upload IPK release package
release arch:
    bash tools/compile/release.sh {{arch}}

# Build with CMake
cmake:
    mkdir -p build
    cd build && cmake .. && make
