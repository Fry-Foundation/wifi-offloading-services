#ifndef PACKAGE_UPDATE_H
#define PACKAGE_UPDATE_H

#include "core/uloop_scheduler.h"
#include "services/access_token.h"
#include "services/device_info.h"
#include "services/registration.h"

typedef struct {
    DeviceInfo *device_info;
    Registration *registration;
    AccessToken *access_token;
    task_id_t task_id;  // Store current task ID for cleanup
} PackageUpdateTaskContext;

PackageUpdateTaskContext *package_update_service(DeviceInfo *device_info,
                                                  Registration *registration,
                                                  AccessToken *access_token);
void clean_package_update_context(PackageUpdateTaskContext *context);
void check_package_update_completion(Registration *registration, DeviceInfo *device_info, AccessToken *access_token);

#endif // PACKAGE_UPDATE_H
