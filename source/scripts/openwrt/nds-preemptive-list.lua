#!/usr/bin/lua

local uci = require("uci").cursor()

--[[
    Function: read_nds_preemptive_clients_list

    Description:
    Retrieves and parses the `preemptive_clients` list from the UCI configuration
    under the `opennds` section. It returns a table of parsed entries, where
    each entry is a table of key-value pairs representing a client's configuration.

    Returns:
    - A table of parsed entries, with each entry containing fields like `mac`,
      `sessiontimeout`, `uploadrate`, `downloadrate`, `uploadquota`, `downloadquota`,
      and `custom`.
    - The raw value of the `preemptive_clients` list from UCI. This value could be a
      a table of strings, or nil (no entries).

    Usage Example:
    local parsed_entries, raw_list = read_nds_preemptive_client_list()
]]
local function read_nds_preemptive_clients_list()
    local preemptive_clients = {}
    local raw_list = uci:get("opennds", "opennds1", "preemptivemac")

    if not raw_list then
        print("No preemptive clients list found")
        return preemptive_clients, nil
    end

    -- In case preemptive_macs is a single string
    if type(raw_list) == "string" then
        preemptive_clients = { raw_list }
    elseif type(raw_list) == "table" then
        preemptive_clients = raw_list
    end

    local parsed_entries = {}

    -- Iterate over the list and parse each entry
    for _, entry in ipairs(preemptive_clients) do
        -- Debug: show the entry
        -- print("Processing entry:", entry)

        local entry_table = {}

        -- Iterate over each key-value pair
        for key, value in string.gmatch(entry, '([^=;]+)=([^;]*)') do
            -- Debug: show the key-value pair
            -- print("Key:", key, "Value:", value)

            -- Remove leading and trailing whitespaces
            key = key:match("^%s*(.-)%s*$")
            value = value:match("^%s*(.-)%s*$")

            -- Add the key-value pair to the entry table
            entry_table[key] = value
        end

        -- Debug: show the parsed table
        -- print("Parsed entry:", entry_table)

        -- Validate the parsed entry and add it to the list
        if entry_table["mac"] then
            table.insert(parsed_entries, entry_table)
        else
            print("Warning: no 'mac' key found in entry " .. entry)
        end
    end

    return parsed_entries, raw_list
end

--[[
    Function: add_nds_preemptive_client

    Description:
    Adds a new client entry to the `preemptive_clients` list in the UCI configuration.
    It checks if the mac address already exists; if not, it appends the new entry
    to the list and commits the changes to UCI.

    Parameters:
    - mac (string): The mac address of the client.
    - sessiontimeout (string): Session timeout for the client.
    - uploadrate (string): Upload rate limit for the client.
    - downloadrate (string): Download rate limit for the client.
    - uploadquota (string): Upload quota limit for the client.
    - downloadquota (string): Download quota limit for the client.
    - custom (string): Custom string for additional client-specific data.

    Usage Example:
    add_nds_preemptive_client("00:11:22:33:44:55", "3600", "100", "100", "100", "100", "custom_data")
]]
local function add_nds_preemptive_client(mac, sessiontimeout, uploadrate, downloadrate, uploadquota, downloadquota,
                                         custom)
    local preemptive_client_list, raw_list = read_nds_preemptive_clients_list()

    -- Check if the mac address is already in the list
    -- If it is, print a message and return
    for _, entry in ipairs(preemptive_client_list) do
        if entry["mac"] == mac then
            print("mac address " .. mac .. " already exists in the list.")
            return
        end
    end

    -- Add the new client to the list
    local value = string.format(
        "mac=%s;sessiontimeout=%s;uploadrate=%s;downloadrate=%s;uploadquota=%s;downloadquota=%s;custom=%s",
        mac, sessiontimeout, uploadrate, downloadrate, uploadquota, downloadquota, custom
    )

    -- Check if preemptive_macs is a table or a string, and add the new entry accordingly
    local uci_list = {}
    if raw_list == nil then
        -- No existing entry, create a new list with the new entry
        uci_list = { value }
    elseif type(raw_list) == "string" then
        -- Only one entry exists, convert it to a table and append the new entry
        uci_list = { raw_list, value }
    elseif type(raw_list) == "table" then
        -- Multiple entries exist, just append the new entry
        table.insert(uci_list, value)
    end

    -- Set the updated value back to UCI
    uci:set("opennds", "opennds1", "preemptivemac", uci_list)

    -- Commit the changes to the UCI configuration
    uci:commit("opennds")

    print("Added new mac address " .. mac .. " to the list.")
end

--[[
    Function: remove_nds_preemptive_client

    Description:
    Removes a client entry from the `preemptive_clients` list in the UCI configuration
    based on the mac address. If the mac address is found, it is removed from the list,
    and the changes are committed to UCI. If the list becomes empty after removal,
    the `preemptive_client` option is deleted from UCI.

    Parameters:
    - mac (string): The mac address of the client to be removed.

    Usage Example:
    remove_nds_preemptive_client("00:11:22:33:44:55")
]]
local function remove_nds_preemptive_client(mac)
    local preemptive_client_list = read_nds_preemptive_clients_list()
    if #preemptive_client_list == 0 then
        return
    end

    local updated_entries = {}
    local found = false

    -- Filter out the entry with the specified mac address
    for _, entry in ipairs(preemptive_client_list) do
        if entry["mac"] ~= mac then
            table.insert(updated_entries, entry)
        else
            found = true
        end
    end

    if not found then
        print("mac address " .. mac .. " not found in the list.")
        return
    end

    -- Reconstruct the updated list as a string and update the uci configuration
    local uci_list = {}
    for _, entry in ipairs(updated_entries) do
        local value = string.format(
            "mac=%s;sessiontimeout=%s;uploadrate=%s;downloadrate=%s;uploadquota=%s;downloadquota=%s;custom=%s",
            entry["mac"], entry["sessiontimeout"], entry["uploadrate"], entry["downloadrate"],
            entry["uploadquota"], entry["downloadquota"], entry["custom"]
        )
        table.insert(uci_list, value)
    end

    -- Update the uci configuration with the new list
    if #uci_list > 0 then
        uci:set("opennds", "opennds1", "preemptivemac", uci_list)
    else
        -- Remove the option if the list is empty
        uci:delete("opennds", "opennds1", "preemptivemac")
    end

    -- Commit the changes to the uci configuration
    uci:commit("opennds")

    print("Removed mac address " .. mac .. " from the list.")
end

-- Main script
local args = { ... }
if #args == 0 then
    print("Usage: lua configure-nds-list.lua <add|remove|read>")
    os.exit(1)
end

if args[1] == "add" then
    if #args ~= 8 then
        print(
            "Usage: lua configure-nds-list.lua add <client_mac> <sessiontimeout> <uploadrate> <downloadrate> <uploadquota> <downloadquota> <custom>")
        os.exit(1)
    end
    add_nds_preemptive_client(args[2], args[3], args[4], args[5], args[6], args[7], args[8])
elseif args[1] == "remove" then
    if #args ~= 2 then
        print("Usage: lua configure-nds-list.lua remove <client_mac>")
        os.exit(1)
    end
    remove_nds_preemptive_client(args[2])
elseif args[1] == "read" then
    local nds_list = read_nds_preemptive_clients_list()
    for _, entry in ipairs(nds_list) do
        print("mac address:", entry["mac"])
        print("session timeout:", entry["sessiontimeout"])
        print("upload rate:", entry["uploadrate"])
        print("download rate:", entry["downloadrate"])
        print("upload quota:", entry["uploadquota"])
        print("download quota:", entry["downloadquota"])
        print("custom:", entry["custom"])
        print("---")
    end
else
    print("Usage: lua configure-nds-list.lua <add|remove|read>")
    os.exit(1)
end
