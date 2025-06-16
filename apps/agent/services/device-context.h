#ifndef DEVICE_CONTEXT_H
#define DEVICE_CONTEXT_H

#include "core/uloop_scheduler.h"
#include "services/access_token.h"
#include "services/registration.h"

typedef struct {
    char *id;
    char *name;
    char *mac;
} Site;

typedef struct {
    Site *site;
} DeviceContext;

typedef struct {
    DeviceContext *device_context;
    Registration *registration;
    AccessToken *access_token;
    task_id_t task_id;  // Store current task ID for cleanup
} DeviceContextTaskContext;

DeviceContext *init_device_context(Registration *registration, AccessToken *access_token);
DeviceContextTaskContext *device_context_service(DeviceContext *device_context,
                                                  Registration *registration,
                                                  AccessToken *access_token);
void clean_device_context_context(DeviceContextTaskContext *context);
void clean_device_context(DeviceContext *device_context);

#endif // DEVICE_CONTEXT_H
