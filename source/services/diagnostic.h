#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include "lib/scheduler.h"
#include "services/device_info.h"

// Initialize the diagnostic service and LEDs
void init_diagnostic_service(void);

// Start the diagnostic service for periodic checks
void start_diagnostic_service(Scheduler *scheduler, DeviceInfo *device_info);

// Update the LED status based on internet connectivity
void update_led_status(bool internet_connected);

#endif // DIAGNOSTIC_H
