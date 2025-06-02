#!/bin/bash

# Define the directory containing the C files
# Target the source directory where C files are located
ROOT_DIR="../../source"

# Check if the source directory exists
if [ ! -d "${ROOT_DIR}" ]; then
    echo "Error: Source directory '${ROOT_DIR}' does not exist!"
    exit 1
fi

# Check if clang-format is available
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed or not in PATH!"
    exit 1
fi

echo "Formatting C/C++ files in ${ROOT_DIR}..."

# Find all C files in the directory and its subdirectories
# and format them using clang-format with the local config file
file_count=0
CLANG_FORMAT_CONFIG="$(pwd)/.clang-format"
find "${ROOT_DIR}" -name "*.c" -or -name "*.h" | while read -r file; do
    if clang-format -style=file:"${CLANG_FORMAT_CONFIG}" -i "$file"; then
        echo "Formatted $file"
        ((file_count++))
    else
        echo "Error formatting $file"
    fi
done

echo "All C files have been formatted."

