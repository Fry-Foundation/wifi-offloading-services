#include "lib/console.h"
#include "lib/scheduler.h"
#include "services/config.h"
#include "services/mqtt.h"
#include <lib/console.h>
#include <mosquitto.h>

struct mosquitto *mosq;

void monitoring_task(Scheduler *sch) {
    // Run the monitoring task
    time_t now = time(NULL);
    char time_str[64];
    ctime_r(&now, time_str);
    publish_mqtt(mosq, "time/topic", time_str);

    // Schedule monitoring_task to rerun later
    schedule_task(sch, time(NULL) + config.monitoring_interval, monitoring_task, "monitoring");
}

void monitoring_service(Scheduler *sch, struct mosquitto *_mosq) {
    mosq = _mosq;
    monitoring_task(sch);
}