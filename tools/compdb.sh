#!/bin/bash

# Change to project root directory
cd "$(dirname "$0")/.."

echo "Generating compilation database with CMake..."

# Create output directory if it doesn't exist
mkdir -p output

# Generate compilation database using CMake
cd output && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. && make

# Copy compile_commands.json to project root for editor support
if [ -f compile_commands.json ]; then
    cp compile_commands.json ..
    echo "Generated compile_commands.json in project root"
else
    echo "Failed to generate compile_commands.json"
    exit 1
fi
