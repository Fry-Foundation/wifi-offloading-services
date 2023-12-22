#!/bin/sh

os_path="/etc/os-release"
os_name=$(grep "^NAME=" "$os_path" | cut -d '"' -f 2)
echo "$os_name"