#!/bin/sh

package_path="$1"

if [ -z "$package_path" ]; then
    echo "-1"
    exit 1
fi

# Stop service
/etc/init.d/wayru-os-services stop

# Upgrade
output=$(opkg upgrade "$package_path" 2>&1)

if echo "$output" | grep -q "upgrading package"; then
    echo "1"
else
    echo "-1"
fi
