#!/bin/sh

rm /etc/config/openwisp
touch /etc/config/openwisp
uci add openwisp controller
uci rename openwisp.@controller[0]=http
uci set openwisp.http.url="$1";
uci set openwisp.http.shared_secret="$2";
uci set openwisp.http.verify_ssl="$3";
uci set openwisp.http.management_interface="$4";
uci set openwisp.http.mac_interface="$5"
uci set openwisp.http.tags="$6";
uci commit openwisp;
/etc/init.d/openwisp_config restart;
/etc/init.d/openwisp-monitoring restart;

