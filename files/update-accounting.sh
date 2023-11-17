#!/bin/sh

data=$(ndsctl json)
echo "$data"

curl --header "Content-Type: application/json" \
  --request PUT \
  --data "$data" \
  https://api.internal.wayru.net/wifi-sessions
