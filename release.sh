#!/bin/bash

# Set these values appropriately
package_name="wayru-os-services"
package_arch="mips_24kc"
version="2.2.12"
file_path="build/2025-03-18-175004/wayru-os-services_2.2.12-1_mips_24kc.ipk"
# api_url="http://localhost:4050/packages/release"
api_url="https://updates.api.internal.wayru.tech/packages/release"
bearer_token="TqohHv9dqnetk3bM4cmGMztR97Qy3PgWesb3Xj72RBLBhAVAx2VCzXTfJ279JsYb"

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
response=$(curl -X POST \
  -H "Authorization: Bearer $bearer_token" \
  -H "Content-Type: multipart/form-data" \
  -F "metadata=$metadata" \
  -F "package=@$file_path" \
  "$api_url")

# Try to format with jq if it's valid JSON, otherwise print raw response
echo "$response" | jq 2>/dev/null || echo "$response"
