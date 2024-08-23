#!/bin/sh

# Include custom binauth script
custom_binauth_path="./nds-binauth-custom.sh"

if [ -e "$custom_binauth_path" ]; then
	$custom_binauth_path "ndsctl_auth" "33:44:55:66:77:88"
	$custom_binauth_path "ndsctl_deauth" "44:55:66:77:88:99"
fi
