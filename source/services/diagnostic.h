#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include "lib/scheduler.h"
#include "services/access_token.h"
#include "services/device_info.h"

// Initialize the diagnostic service and LEDs
void init_diagnostic_service(DeviceInfo *device_info);

// Run init diagnostics
void run_init_diagnostics();

// Start the diagnostic service for periodic checks
void start_diagnostic_service(Scheduler *scheduler, AccessToken *access_token);

// Update the LED status based on internet connectivity
void update_led_status(bool ok, const char *context);

#endif // DIAGNOSTIC_H
