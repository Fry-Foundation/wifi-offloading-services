#!/bin/bash

# Define the directory containing the C files
# You can change this to the root directory of your repository
ROOT_DIR="."

# Find all C files in the directory and its subdirectories
# and format them using clang-format
find "${ROOT_DIR}" -name "*.c" -or -name "*.h" | while read -r file; do
    clang-format -i "$file"
    echo "Formatted $file"
done

echo "All C files have been formatted."

