#include "device_status.h"
#include "lib/scheduler.h"
#include "services/config.h"
#include <stdbool.h>
#include <stdlib.h>

DeviceStatus device_status = Unknown;

bool on_boot = true;

DeviceStatus request_device_status() { return Unknown; }

void device_status_task(Scheduler *sch) {
    device_status = request_device_status();
    schedule_task(&sch, time(NULL) + config.device_status_interval, device_status_task, "device status");
}

void init_device_status_service(Scheduler *sch) {
    device_status_task(&sch);

    // Side effects
    // Make sure wayru operator is running (all status codes but 6)
    // Start the peaq did service (on status 5)
    // Check that the captive portal is running (on status 6)
    // Disable wayru operator network (on status 6)
}
