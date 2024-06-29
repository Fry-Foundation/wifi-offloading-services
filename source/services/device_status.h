#ifndef DEVICE_STATUS_H
#define DEVICE_STATUS_H

#include "lib/scheduler.h"
#include <stdbool.h>

typedef enum {
    Unknown = -1,
    Initial = 0,
    SetupPending = 1,
    SetupApproved = 2,
    MintPending = 3,
    Ready = 4,
    Banned = 5,
} DeviceStatus;

extern DeviceStatus device_status;

void device_status_service(Scheduler *sch);

#endif /* DEVICE_STATUS_H */
