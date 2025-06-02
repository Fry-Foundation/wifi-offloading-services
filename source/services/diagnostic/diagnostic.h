#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include "lib/scheduler.h"
#include "services/access_token.h"
#include "services/device_info.h"
#include <stdbool.h>

// Initialize the diagnostic service and run all init tests
bool init_diagnostic_service(DeviceInfo *device_info);

// Comprehensive DNS resolution check for all critical domains
bool comprehensive_dns_check();

// Comprehensive API health check for all Wayru APIs
bool comprehensive_api_health_check();

// DNS resolution check with retry logic (single host)
bool dns_resolve_check(const char *host);

// Start the diagnostic service for periodic checks
void start_diagnostic_service(Scheduler *scheduler, AccessToken *access_token);

// Update the LED status based on internet connectivity
void update_led_status(bool ok, const char *context);

// Network check functions
bool internet_check(const char *host);
bool wayru_check();

// Internal diagnostic task function
void diagnostic_task(Scheduler *sch, void *task_context);

#endif // DIAGNOSTIC_H
