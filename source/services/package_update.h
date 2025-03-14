#ifndef PACKAGE_UPDATE_H
#define PACKAGE_UPDATE_H

#include "lib/scheduler.h"
#include "services/access_token.h"
#include "services/device_info.h"
#include "services/registration.h"

void package_update_service(Scheduler *sch, DeviceInfo *device_info, Registration *registration, AccessToken *access_token);
void check_package_update_completion(Registration *registration, DeviceInfo *device_info, AccessToken *access_token);

#endif // PACKAGE_UPDATE_H
