#ifndef SETUP_H
#define SETUP_H

#include "lib/scheduler.h"
#include "services/device_info.h"

void setup_service(Scheduler *sch, DeviceInfo *device_info, char *wayru_device_id);

#endif // SETUP_H
