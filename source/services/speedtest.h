#ifndef SPEEDTEST_H
#define SPEEDTEST_H

#include <mosquitto.h>
#include "services/registration.h"

void speed_test();
void speedtest_service(struct mosquitto *_mosq, Registration *_registration);
#endif