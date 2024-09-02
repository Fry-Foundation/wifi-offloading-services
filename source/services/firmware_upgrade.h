#ifndef FIRMWARE_UPGRADE_H
#define FIRMWARE_UPGRADE_H

#include "lib/scheduler.h"
#include "services/device_info.h"
#include "services/registration.h"

void firmware_upgrade_on_boot(Registration *registration, DeviceInfo *device_info);
void firmware_upgrade_check(Scheduler *scheduler, DeviceInfo *device_info, Registration *registration);
void send_firmware_check_request(const char *codename, const char *version, const char *wayru_device_id);
void clean_firmware_upgrade_service();

#endif // FIRMWARE_UPGRADE_H
