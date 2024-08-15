#!/usr/bin/lua

local uci = require("uci").cursor()

-- Main script
local args = { ... }
if #args ~= 1 then
    print("Usage: lua network-mac.lua <site_mac>")
    os.exit(1)
end

local site_mac = args[1]

-- Update the uci configuration with the new list
uci:set("network", "device1", "macaddr", site_mac)
uci:commit("network")
os.execute("service network restart")

print("Configured site mac address:", site_mac)
