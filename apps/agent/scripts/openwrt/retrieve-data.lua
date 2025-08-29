#!/usr/bin/lua 

local io = require('io')

--utils
function get_cpus()
  local processors_file = io.popen('cat /proc/cpuinfo | grep -c processor')
  local processors = processors_file:read('*a')
  processors_file:close()
  local cpus = tonumber(processors)
  -- assume the hardware has at least 1 proc
  if cpus == 0 then return 1 end
  return cpus
end

function split(str, pat)
  local t = {}
  local fpat = "(.-)" .. pat
  local last_end = 1
  local s, e, cap = str:find(fpat, 1)
  while s do
    if s ~= 1 or cap ~= "" then table.insert(t, cap) end
    last_end = e + 1
    s, e, cap = str:find(fpat, last_end)
  end
  if last_end <= #str then
    cap = str:sub(last_end)
    table.insert(t, cap)
  end
  return t
end

function parse_disk_usage()
  local disk_usage_info = {}
  local disk_usage_file = io.popen('df')
  local disk_usage = disk_usage_file:read("*a")
  disk_usage_file:close()
  for _, line in ipairs(split(disk_usage, "\n")) do
    if line:sub(1, 10) ~= 'Filesystem' then
      local filesystem, size, used, available, percent, location = line:match(
        '(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)')
      if filesystem ~= 'tmpfs' and not string.match(filesystem, 'overlayfs') then
        percent = percent:gsub('%W', '')
        -- available, size and used are in KiB
        table.insert(disk_usage_info, {
          filesystem = filesystem,
          available_bytes = tonumber(available) * 1024,
          size_bytes = tonumber(size) * 1024,
          used_bytes = tonumber(used) * 1024,
          used_percent = tonumber(percent),
          mount_point = location
        })
      end
    end
  end
  return disk_usage_info
end

-- get memory info
local function get_memory_info()
  local memory_info = {}
  local handle = io.popen("cat /proc/meminfo")
  local result = handle:read("*a")
  handle:close()

  -- Parse the output of `cat /proc/meminfo`
  for line in result:gmatch("[^\r\n]+") do
      local key, value = line:match("([^:]+):%s+(%d+)")
      if key and value then
          key = key:gsub(" ", "_"):lower()
          memory_info[key] = tonumber(value)
      end
  end

  -- Calculate additional memory metrics
  memory_info.memory_total = memory_info.memtotal * 1024
  memory_info.memory_free = memory_info.memfree * 1024
  memory_info.memory_used = (memory_info.memory_total - memory_info.memory_free)
  memory_info.memory_shared = memory_info.shmem * 1024
  memory_info.memory_buffered = memory_info.buffers * 1024

  return memory_info
end

local system_info = get_memory_info()
local memoryTotal = system_info.memory_total
local memoryFree = system_info.memory_free
local memoryUsed = system_info.memory_used
local memoryShared = system_info.memory_shared
local memoryBuffered = system_info.memory_buffered

-- get disk info
local disks = parse_disk_usage()
local disk_data

for _, disk in ipairs(disks) do
    if(disk.mount_point == '/overlay') then
      disk_data = disk
    end
end

local diskUsed = disk_data.used_bytes
local diskSize = disk_data.size_bytes
local diskAvailable = disk_data.available_bytes
local diskUsedPercent = disk_data.used_percent

-- get cpu count, cpu load and load percent
function get_cpu_load_1min()
    local file = io.open("/proc/loadavg", "r")
    if not file then
        print("Could not open /proc/loadavg")
        return nil
    end

    local load = file:read("*a")
    file:close()

    local load1 = load:match("(%S+)")

    return tonumber(load1)
end
local cpu_count = get_cpus()
local cpuLoad = get_cpu_load_1min()
local cpuLoadPercent = 100 * (cpuLoad / cpu_count)
 
-- get wifi clients
local handle = io.popen("iwinfo | grep ESSID | awk '{print $1}'")
local interfaces = handle:read("*a")
handle:close()

local function count_clients(interface)
    local handle = io.popen("iwinfo " .. interface .. " assoclist | grep '^[A-F0-9]' | wc -l")
    local count = handle:read("*a")
    handle:close()
    return tonumber(count)
end

local wifiClients = 0
for interface in interfaces:gmatch("%S+") do
    local count = count_clients(interface)
    wifiClients = wifiClients + count
end

-- get radio count and radio live
local function get_wiphy_list()
    local wiphy_list = {}
    local handle = io.popen("iw phy")
    for line in handle:lines() do
        local wiphy = line:match("^Wiphy%s+(%S+)")
        if wiphy then
            table.insert(wiphy_list, wiphy)
        end
    end
    handle:close()
    return wiphy_list
end

local function has_active_interfaces(wiphy)
    local command = "iwinfo | grep " .. wiphy
    local handle = io.popen(command)
    local result = handle:read("*a")
    handle:close()
    return result ~= ""
end

local wiphy_list = get_wiphy_list()
local radioCount = #wiphy_list
local radioLive = 0

for _, wiphy in ipairs(wiphy_list) do
    if has_active_interfaces(wiphy) then
        radioLive = radioLive + 1
    end
end

-- print results
print("wifi_clients:", wifiClients)
print("memory_total:", memoryTotal)
print("memory_free:", memoryFree)
print("memory_used:", memoryUsed)
print("memory_shared:", memoryShared)
print("memory_buffered:", memoryBuffered)
print("cpu_count:", cpu_count)
print("cpu_load:", cpuLoad)
print("cpu_load_percent:", cpuLoadPercent)
print("disk_used:",diskUsed)
print("disk_size:", diskSize)
print("disk_available:", diskAvailable)
print("disk_used_percent:", diskUsedPercent)
print("radio_count:", radioCount)
print("radio_live:", radioLive)
