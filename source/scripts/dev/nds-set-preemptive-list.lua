#!/usr/bin/lua

-- In-memory storage for preemptive clients list
-- @note you can use the mac address below to test the remove function
local preemptive_clients = {
    "mac=00:11:22:33:44:55;sessiontimeout=3600;uploadrate=100;downloadrate=100;uploadquota=100;downloadquota=100;custom=custom_data",
}

--[[
    Function: read_nds_preemptive_clients_list

    Description:
    Retrieves and parses the `preemptive_clients` list. Since we're not using UCI,
    this function simply returns the current in-memory list.

    Returns:
    - A table of parsed entries, where each entry contains fields like `mac`,
      `sessiontimeout`, `uploadrate`, `downloadrate`, `uploadquota`, `downloadquota`,
      and `custom`.
    - The raw list, which is a table of strings representing each client's configuration.

    Usage Example:
    local parsed_entries, raw_list = read_nds_preemptive_clients_list()
]]
local function read_nds_preemptive_clients_list()
    local parsed_entries = {}

    -- Iterate over the in-memory list and parse each entry
    for _, entry in ipairs(preemptive_clients) do
        local entry_table = {}

        -- Parse key-value pairs from the entry string
        for key, value in string.gmatch(entry, '([^=;]+)=([^;]*)') do
            key = key:match("^%s*(.-)%s*$")
            value = value:match("^%s*(.-)%s*$")
            entry_table[key] = value
        end

        -- Add the parsed entry to the list if it has a mac address
        if entry_table["mac"] then
            table.insert(parsed_entries, entry_table)
        else
            print("Warning: no 'mac' key found in entry " .. entry)
        end
    end

    return parsed_entries, preemptive_clients
end

--[[
    Function: add_nds_preemptive_client

    Description:
    Adds a new client entry to the `preemptive_clients` list. It checks if the mac address
    already exists; if not, it appends the new entry to the list.

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
    local parsed_entries = read_nds_preemptive_clients_list()

    -- Check if the mac address is already in the list
    for _, entry in ipairs(parsed_entries) do
        if entry["mac"] == mac then
            print("mac address " .. mac .. " already exists in the list.")
            return
        end
    end

    -- Add the new client to the in-memory list
    local value = string.format(
        "mac=%s;sessiontimeout=%s;uploadrate=%s;downloadrate=%s;uploadquota=%s;downloadquota=%s;custom=%s",
        mac, sessiontimeout, uploadrate, downloadrate, uploadquota, downloadquota, custom
    )
    table.insert(preemptive_clients, value)

    print("Added new mac address " .. mac .. " to the list.")
end

--[[
    Function: remove_nds_preemptive_client

    Description:
    Removes a client entry from the `preemptive_clients` list based on the mac address.
    If the mac address is found, it is removed from the list.

    Parameters:
    - mac (string): The mac address of the client to be removed.

    Usage Example:
    remove_nds_preemptive_client("00:11:22:33:44:55")
]]
local function remove_nds_preemptive_client(mac)
    local parsed_entries, _ = read_nds_preemptive_clients_list()
    local updated_entries = {}
    local found = false

    -- Filter out the entry with the specified mac address
    for _, entry in ipairs(parsed_entries) do
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

    -- Rebuild the in-memory list
    preemptive_clients = {}
    for _, entry in ipairs(updated_entries) do
        local value = string.format(
            "mac=%s;sessiontimeout=%s;uploadrate=%s;downloadrate=%s;uploadquota=%s;downloadquota=%s;custom=%s",
            entry["mac"], entry["sessiontimeout"], entry["uploadrate"], entry["downloadrate"],
            entry["uploadquota"], entry["downloadquota"], entry["custom"]
        )
        table.insert(preemptive_clients, value)
    end

    print("Removed mac address " .. mac .. " from the list.")
end

-- Main script
local args = { ... }
if #args == 0 then
    print("Usage: lua nds-set-preemptive-list.lua <add|remove|read>")
    os.exit(1)
end

if args[1] == "add" then
    if #args ~= 8 then
        print(
            "Usage: lua nds-set-preemptive-list.lua add <client_mac> <sessiontimeout> <uploadrate> <downloadrate> <uploadquota> <downloadquota> <custom>")
        os.exit(1)
    end
    add_nds_preemptive_client(args[2], args[3], args[4], args[5], args[6], args[7], args[8])
elseif args[1] == "remove" then
    if #args ~= 2 then
        print("Usage: lua nds-set-preemptive-list.lua remove <client_mac>")
        os.exit(1)
    end
    remove_nds_preemptive_client(args[2])
elseif args[1] == "read" then
    local parsed_entries = read_nds_preemptive_clients_list()
    for _, entry in ipairs(parsed_entries) do
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
    print("Usage: lua nds-set-preemptive-list.lua <add|remove|read>")
    os.exit(1)
end
