# Default recipe
default:
    @just --list

# Development environment setup
dev:
    just cmake
    bash tools/dev.sh

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
    mkdir -p output
    cd output && cmake .. && make