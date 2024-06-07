#ifndef DEVICE_STATUS_H
#define DEVICE_STATUS_H

#include "lib/scheduler.h"
#include <stdbool.h>

typedef enum {
    Unknown = -1,
    Initial = 0,
    SetupPending = 2,
    SetupApproved = 3,
    MintPending = 4,
    Ready = 6,
    Banned = 7,
} DeviceStatus;

bool on_boot;

extern DeviceStatus device_status;

void init_device_status_service(Scheduler *sch);

#endif /* DEVICE_STATUS_H */
