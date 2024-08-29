#ifndef COMMANDS_H
#define COMMANDS_H

#include "mosquitto.h"
#include "device_info.h"
#include "registration.h"

void commands_service(struct mosquitto *mosq, DeviceInfo *device_info, Registration *registration);

#endif /* COMMANDS_H */
