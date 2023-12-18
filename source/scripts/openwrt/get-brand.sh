#!/bin/sh

brand_file="/etc/board.json"
brand=$(grep -o '"id": "[^"]*"' "$brand_file" | sed 's/"id": "//' | sed 's/,.*//')
echo "$brand"
