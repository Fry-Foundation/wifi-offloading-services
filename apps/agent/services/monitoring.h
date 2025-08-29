#ifndef MONITORING_H
#define MONITORING_H

#include "core/uloop_scheduler.h"
#include "services/registration.h"
#include <mosquitto.h>

typedef struct {
    struct mosquitto *mosq;
    Registration *registration;
    char *os_name;
    char *os_version;
    char *os_services_version;
    char *public_ip;
    task_id_t task_id; // Store current task ID for cleanup
} MonitoringTaskContext;

MonitoringTaskContext *monitoring_service(struct mosquitto *mosq, Registration *registration);
void clean_monitoring_context(MonitoringTaskContext *context);

#endif /* MONITORING_H */
