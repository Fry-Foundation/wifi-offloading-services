#ifndef MONITORING_H
#define MONITORING_H

#include "lib/scheduler.h"
#include "services/registration.h"
#include <mosquitto.h>

void monitoring_service(Scheduler *sch, struct mosquitto *mosq, Registration *registration);

#endif /* MONITORING_H */
