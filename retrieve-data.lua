#!/usr/bin/lua 

local ubus_lib = require('ubus')
local io = require('io')
local monitoring = require('openwisp-monitoring.monitoring')

local ubus = ubus_lib.connect()
if not ubus then error('Failed to connect to ubusd') end


local system_info = ubus:call('system', 'info', {})
local cpu_count = monitoring.resources.get_cpus()
local memoryTotal = system_info.memory.total
local memoryFree = system_info.memory.free
local memoryUsed = memoryTotal - memoryFree
local memoryShared = system_info.memory.shared
local memoryBuffered = system_info.memory.buffered

local disks = monitoring.resources.parse_disk_usage()
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

print("memory Total:", memoryTotal)
print("memory Free: ", memoryFree)
print("memory Used: ", memoryUsed)
print("memory Shared: ", memoryShared)
print("memory Buffered: ", memoryBuffered)
print("CPU Count: ", cpu_count)
print("Disk Used: ",diskUsed)
print("Disk Size: ", diskSize)
print("Disk Available: ", diskAvailable)
print("Disk Used Percent: ", diskUsedPercent)

