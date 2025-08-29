#!/bin/sh

public_ip=$(curl -s ifconfig.me)
echo "$public_ip"