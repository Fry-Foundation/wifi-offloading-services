#ifndef ACCESS_H
#define ACCESS_H

#include "lib/scheduler.h"
#include "services/device_info.h"
#include <time.h>

typedef struct {
    char *public_key;
    time_t issued_at_seconds;
    time_t expires_at_seconds;
} AccessKey;

extern AccessKey access_key;

void access_service(Scheduler *sch, DeviceInfo *_device_info);
void clean_access_service();

#endif /* ACCESS_H */
