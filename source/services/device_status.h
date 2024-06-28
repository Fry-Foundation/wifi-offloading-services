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
    Ready = 5,
    Banned = 6,
} DeviceStatus;

extern DeviceStatus device_status;

void device_status_service(Scheduler *sch);

#endif /* DEVICE_STATUS_H */
