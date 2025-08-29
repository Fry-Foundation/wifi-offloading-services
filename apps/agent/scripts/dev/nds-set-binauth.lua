#!/usr/bin/lua

-- Main script
local args = { ... }
if #args ~= 1 then
    print("Usage: lua nds-set-binauth.lua <script_path>")
    os.exit(1)
end

local script_path = args[1]

print("Configured binauth script path:", script_path)
