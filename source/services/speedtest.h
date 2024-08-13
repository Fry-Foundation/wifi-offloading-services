#ifndef SPEEDTEST_H
#define SPEEDTEST_H

#include "services/access_token.h"
#include "services/registration.h"
#include <mosquitto.h>

void speed_test();
void speedtest_service(struct mosquitto *mosq, Registration *registration, AccessToken *access_token);

#endif // SPEEDTEST_H
