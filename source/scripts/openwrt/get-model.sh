#!/bin/sh

model_file="/etc/board.json"
model=$(grep -o '"id": "[^"]*"' "$model_file" | sed 's/"id": ".*,//' | sed 's/"//')
echo "$model"