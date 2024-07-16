#include "lib/scheduler.h"
#include "services/config.h"
#include "services/mqtt.h"
#include "lib/console.h"
#include <mosquitto.h>

void monitoring_task(Scheduler *sch, void *mosq) {
    // Run the monitoring task
    // run_monitoring();
    time_t now = time(NULL);

    char time_str[64];
    ctime_r(&now, time_str);

    console(CONSOLE_INFO, "Monitoring task ran");
    console(CONSOLE_INFO, "Monitoring interval: %d", config.monitoring_interval);
    publish_mqtt(mosq,"time/topic", time_str);
    // Schedule monitoring_task to rerun later
    schedule_task(sch, time(NULL) + config.monitoring_interval, monitoring_task, mosq, "monitoring");
}

void monitoring_service(Scheduler *sch, struct mosquitto * mosq) { monitoring_task(sch, mosq); }