#!/bin/sh

package_path="$1"

if [ -z "$package_path" ]; then
    echo "-1"
    exit 1
fi

# Write a temporary upgrade script
cat > /tmp/do_upgrade.sh << EOF
#!/bin/sh
sleep 5  # Give the parent process time to exit
opkg remove wayru-os-services
opkg install "$package_path"
EOF

chmod +x /tmp/do_upgrade.sh
/tmp/do_upgrade.sh > /tmp/upgrade.log 2>&1 &

echo "Upgrade initiated"
exit 0
