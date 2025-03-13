#!/bin/bash

# Set these values appropriately
package_name="wayru-os-services"
package_arch="mips_24kc"
version="2.2.9"
file_path="build/2025-03-13-152222/wayru-os-services_2.2.9-1_mips_24kc.ipk"
api_url="http://localhost:4050/packages/release"

# Verify the file exists
if [ ! -f "$file_path" ]; then
    echo "Error: File '$file_path' does not exist."
    exit 1
fi

# Automatically calculate the SHA-256 checksum
checksum="$(sha256sum "$file_path" | cut -d' ' -f1)"
echo "Calculated checksum: $checksum"

# Automatically determine the file size in bytes
size_bytes=$(stat -c%s "$file_path")
echo "File size: $size_bytes bytes"

# Create JSON metadata
metadata="{\"package_name\":\"$package_name\",\"package_architecture_name\":\"$package_arch\",\"version\":\"$version\",\"storage_path\":\"$file_path\",\"checksum\":\"$checksum\",\"size_bytes\":$size_bytes}"

echo "Sending package release request..."
echo "Metadata: $metadata"
echo "File: $file_path"
echo "API URL: $api_url"

# Execute the curl command
curl -X POST \
  -H "Content-Type: multipart/form-data" \
  -F "metadata=$metadata" \
  -F "package=@$file_path" \
  "$api_url" | jq

echo "Request completed."
