#!/bin/sh

TAR_FILE="$1/firmware.tar.gz"           
EXTRACT_DIR="$1/extracted_files"   
HASH_FILE="$EXTRACT_DIR/sha256sums"   

mkdir -p "$EXTRACT_DIR"

# Extract files from .tar.gz archive
tar -xzf "$TAR_FILE" -C "$EXTRACT_DIR"

# Find the .bin file in the extracted directory
IMAGE_FILE=$(find "$EXTRACT_DIR" -name "*.bin")

# Check if files exist after extraction
if [ -z "$IMAGE_FILE" ]; then
    echo "Error: No .bin file found in '$EXTRACT_DIR'."
    exit -1
fi

if [ ! -f "$HASH_FILE" ]; then
    echo "Error: Hash file '$HASH_FILE' does not exist."
    exit -1
fi

# Calculate the hash of the image
CALCULATED_HASH=$(sha256sum "$IMAGE_FILE" | awk '{ print $1 }')

# Read the provided hash
PROVIDED_HASH=$(awk '{ print $1 }' "$HASH_FILE")

# Compare hashes
if [ "$CALCULATED_HASH" = "$PROVIDED_HASH" ]; then
    # Move the image to /tmp
    mv "$IMAGE_FILE" "$1/firmware.bin"
    echo 1

else
    echo -1
fi
