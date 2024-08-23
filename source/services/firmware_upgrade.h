#ifndef FIRMWARE_UPGRADE_H
#define FIRMWARE_UPGRADE_H

#include "services/device_info.h"
#include "lib/scheduler.h"
#include "services/registration.h"

void firmware_upgrade_check(Scheduler *scheduler, const DeviceInfo *device_info, const Registration *registration);
void clean_firmware_upgrade_service();
void firmware_upgrade_on_boot(Registration *registration, DeviceInfo *device_info);


#endif // FIRMWARE_UPGRADE_H
