brand_model_file="/etc/board.json"

mac_address="/sys/devices/virtual/net/br-lan/address"

brand_model=$(grep -o '"id": "[^"]*"' "$brand_model_file" | awk -F ': ' '{print $2}' | tr -d '"')
model=$(echo "$brand_model" | sed 's/,/-/')
mac=$(cat ${mac_address})
id="${model}-${mac}"
echo "$id"

