#!/usr/bin/lua

-- Main script
local args = { ... }
if #args ~= 1 then
    print("Usage: lua network-set-mac.lua <site_mac>")
    os.exit(1)
end

local site_mac = args[1]
print("Configured site mac address:", site_mac)
