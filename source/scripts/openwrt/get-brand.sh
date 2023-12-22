#!/bin/sh

#brand_file="/etc/board.json"
#brand=$(grep -o '"id": "[^"]*"' "$brand_file" | sed 's/"id": "//' | sed 's/,.*//')
#echo "$brand"

# Rutas de los archivos
brand_file1="/etc/os-release"
brand_file2="/etc/board.json" 

# Obtener el valor de OPENWRT_DEVICE_MANUFACTURER de brand_file1
brand=$(grep "^OPENWRT_DEVICE_MANUFACTURER=" "$brand_file1" | cut -d '"' -f 2)

# Comprobar si el valor es diferente de "OpenWrt"
if [ "$brand" != "OpenWrt" ]; then
    echo "$brand"
else
    # Si el valor es "OpenWrt", buscar el valor correspondiente en brand_file2 

    if [ "$brand" = "OpenWrt" ]; then
        brand=$(grep -o '"id": "[^"]*"' "$brand_file2" | sed 's/"id": "//' | sed 's/,.*//')
        echo "$brand"    
    fi
fi

