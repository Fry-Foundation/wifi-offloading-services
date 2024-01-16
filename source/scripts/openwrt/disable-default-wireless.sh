#!/bin/sh

uci set wireless.default_wifi_interface_0.disabled=1
uci set wireless.default_wifi_interface_1.disabled=1
uci commit wireless

echo "done"