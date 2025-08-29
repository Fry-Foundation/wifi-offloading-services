#ifndef COMMANDS_H
#define COMMANDS_H

#include "mosquitto.h"
#include "services/access_token.h"
#include "services/device_info.h"
#include "services/registration.h"

void commands_service(struct mosquitto *mosq,
                      DeviceInfo *device_info,
                      Registration *registration,
                      AccessToken *access_token);

#endif /* COMMANDS_H */
