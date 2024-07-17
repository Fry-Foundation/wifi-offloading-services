#ifndef MONITORING_H
#define MONITORING_H

#include "lib/scheduler.h"
#include <mosquitto.h>

void monitoring_service(Scheduler *sch, struct mosquitto *mosq);

#endif /* MONITORING_H */
