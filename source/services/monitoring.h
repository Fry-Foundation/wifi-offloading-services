#ifndef MONITORING_H
#define MONITORING_H

#include "lib/scheduler.h"
#include "services/device_info.h"
#include <mosquitto.h>

void monitoring_service(Scheduler *sch, struct mosquitto *_mosq, DeviceInfo *_device_info);

#endif /* MONITORING_H */
