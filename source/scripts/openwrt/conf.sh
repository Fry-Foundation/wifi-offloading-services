#!/bin/sh

if [ "$1" = '' ]; then
    enabled="1"
else
    enabled="$1"
fi

if [ "$2" = '' ]; then
    main_api="https://api.wayru.tech"
else
    main_api="$2"
fi

if [ "$3" = '' ]; then
    accounting_api="https://api.wifi.wayru.tech"
else
    accounting_api="$3"
fi