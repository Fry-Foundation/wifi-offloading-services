#!/usr/bin/env lua

local json = require("json")
local uci = require("uci")

-- Initialize UCI cursor
local cursor = uci.cursor()

-- Helper function to convert JSON boolean to UCI string
local function bool_to_uci(value)
    if type(value) == "boolean" then
        return value and "1" or "0"
    end
    return tostring(value)
end

-- Apply wireless interface configuration
local function apply_wireless_interface(interface_config)
    local name = interface_config.name
    if not name then
        print("Error: Interface missing required 'name' field")
        return false
    end

    print("Configuring wireless interface: " .. name)

    -- Set each property from the interface configuration
    for key, value in pairs(interface_config) do
        if key ~= "name" then -- Skip the name field as it's the section identifier
            local uci_value
            if type(value) == "boolean" then
                uci_value = bool_to_uci(value)
            else
                uci_value = tostring(value)
            end

            local success, err = pcall(function()
                cursor:set("wireless", name, key, uci_value)
            end)

            if not success then
                print("Error setting wireless." ..
                name .. "." .. key .. "=" .. uci_value .. ": " .. (err or "unknown error"))
                return false
            end
        end
    end

    print("Successfully configured interface: " .. name)
    return true
end

-- Apply all wireless interfaces from configuration
local function apply_wireless_interfaces(config)
    local wireless = config.wireless
    if not wireless then
        print("No wireless configuration found")
        return false
    end

    local interfaces = wireless.interfaces
    if not interfaces or type(interfaces) ~= "table" then
        print("No wireless interfaces found")
        return false
    end

    -- Apply each interface configuration
    for _, interface_config in ipairs(interfaces) do
        if not apply_wireless_interface(interface_config) then
            return false
        end
    end

    return true
end

-- Read and parse JSON configuration file
local function read_config(filename)
    local file, err = io.open(filename, "r")
    if not file then
        print("Error: Cannot open config file '" .. filename .. "': " .. (err or "unknown error"))
        return nil
    end

    local content = file:read("*all")
    file:close()

    local success, config = pcall(json.decode, content)
    if not success then
        print("Error: Failed to parse JSON: " .. (config or "unknown error"))
        return nil
    end

    return config
end

-- Main function
local function main(args)
    local config_file = args[1] or "config.example.json"

    -- Read configuration
    local root_config = read_config(config_file)
    if not root_config then
        return 1
    end

    local config = root_config.config
    if not config then
        print("Error: No 'config' object found in JSON")
        return 1
    end

    -- Apply wireless configuration
    if not apply_wireless_interfaces(config) then
        print("Error: Failed to apply wireless configuration")
        return 1
    end

    -- Save changes
    local success, err = pcall(function()
        cursor:save("wireless")
    end)

    if not success then
        print("Error: Failed to save wireless configuration: " .. (err or "unknown error"))
        return 1
    end

    -- Commit changes
    success, err = pcall(function()
        cursor:commit("wireless")
    end)

    if not success then
        print("Error: Failed to commit wireless configuration: " .. (err or "unknown error"))
        return 1
    end

    print("Wireless configuration applied and committed successfully")
    return 0
end

-- Run main function and exit with appropriate code
os.exit(main(arg))
