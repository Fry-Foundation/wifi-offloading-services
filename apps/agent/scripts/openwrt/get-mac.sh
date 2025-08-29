#!/bin/sh

MAC_ADDRESS_FILE="/sys/devices/virtual/net/br-lan/address"

get_address_from_interface() {
  interface_address=$(cat ${MAC_ADDRESS_FILE})
  mac_address=${interface_address%"\n"}

  echo "${mac_address}"
}

get_address_from_interface
