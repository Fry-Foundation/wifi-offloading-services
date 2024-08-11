#ifndef DEVICE_CONTEXT_H
#define DEVICE_CONTEXT_H

#include "lib/scheduler.h"
#include "services/access_token.h"
#include "services/registration.h"

typedef struct {
    char *site;
} DeviceContext;

DeviceContext *init_device_context(Registration *registration, AccessToken *access_token);
void device_context_service(Scheduler *sch,
                            DeviceContext *device_context,
                            Registration *registration,
                            AccessToken *access_token);
void clean_device_context(DeviceContext *device_context);

#endif // DEVICE_CONTEXT_H
