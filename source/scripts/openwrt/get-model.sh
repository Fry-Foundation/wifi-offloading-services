#!/bin/sh

#model_file="/etc/board.json"
#model=$(grep -o '"id": "[^"]*"' "$model_file" | sed 's/"id": ".*,//' | sed 's/"//')
#echo "$model"

# Rutas de los archivos
model_file1="/etc/os-release"
model_file2="/etc/board.json" 

# Obtener el valor de OPENWRT_DEVICE_PRODUCT de model_file1
model=$(grep "^OPENWRT_DEVICE_PRODUCT=" "$model_file1" | cut -d '"' -f 2)

# Comprobar si el valor es diferente de "Generic"
if [ "$model" != "Generic" ]; then
    echo "$model"
else
    # Si el valor es "Generic", buscar el valor correspondiente en model_file2 

    if [ "$model" = "Generic" ]; then
        model=$(grep -o '"id": "[^"]*"' "$model_file2" | sed 's/"id": ".*,//' | sed 's/"//')
        echo "$model"    
    fi
fi