#ifndef DEVICE_CONTEXT_H
#define DEVICE_CONTEXT_H

#include "services/registration.h"
#include "services/access_token.h"

typedef struct {
    char *site;
} DeviceContext;

DeviceContext *init_device_context(Registration *registration, AccessToken *access_token);

#endif // DEVICE_CONTEXT_H
