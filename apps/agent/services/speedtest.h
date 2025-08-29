#ifndef SPEEDTEST_H
#define SPEEDTEST_H

#include "core/uloop_scheduler.h"
#include "services/access_token.h"
#include "services/registration.h"
#include <mosquitto.h>

typedef struct {
    struct mosquitto *mosq;
    Registration *registration;
    AccessToken *access_token;
    task_id_t task_id; // Store current task ID for cleanup
} SpeedTestTaskContext;

SpeedTestTaskContext *speedtest_service(struct mosquitto *mosq, Registration *registration, AccessToken *access_token);
void clean_speedtest_context(SpeedTestTaskContext *context);

#endif // SPEEDTEST_H
