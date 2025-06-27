#!/bin/bash

# Define the directories containing the C files
# Target the source directories where C files are located
APPS_DIR="../../apps"
LIB_DIR="../../lib"

# Check if the source directories exist
if [ ! -d "${APPS_DIR}" ]; then
    echo "Error: Source directory '${APPS_DIR}' does not exist!"
    exit 1
fi

if [ ! -d "${LIB_DIR}" ]; then
    echo "Error: Source directory '${LIB_DIR}' does not exist!"
    exit 1
fi

# Check if clang-format is available
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed or not in PATH!"
    exit 1
fi

echo "Formatting C/C++ files in ${APPS_DIR} and ${LIB_DIR}..."

# Find all C files in the directories and their subdirectories
# and format them using clang-format with the local config file
file_count=0
CLANG_FORMAT_CONFIG="$(pwd)/.clang-format"

for dir in "${APPS_DIR}" "${LIB_DIR}"; do
    echo "Processing directory: ${dir}"
    find "${dir}" -name "*.c" -or -name "*.h" | while read -r file; do
        if clang-format -style=file:"${CLANG_FORMAT_CONFIG}" -i "$file"; then
            echo "Formatted $file"
            ((file_count++))
        else
            echo "Error formatting $file"
        fi
    done
done

echo "All C files have been formatted."
