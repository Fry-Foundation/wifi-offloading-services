#!/usr/bin/lua

local uci = require("uci").cursor()

-- Main script
local args = { ... }
if #args ~= 1 then
    print("Usage: lua nds-set-binauth.lua <script_path>")
    os.exit(1)
end

local script_path = args[1]

-- Update the uci configuration with the new list
uci:set("opennds", "opennds1", "binauth", script_path)
uci:commit("opennds")
os.execute("service opennds restart")

print("Configured binauth script path:", script_path)
