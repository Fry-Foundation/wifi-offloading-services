brand_model_file="/etc/board.json"
brand_model=$(grep -o '"id": "[^"]*"' "$brand_model_file" | awk -F ': ' '{print $2}' | tr -d '"')
model=$(echo "$brand_model" | sed 's/,/-/')
echo "$model"