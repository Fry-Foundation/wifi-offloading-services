#include "diagnostic.h"
#include "lib/console.h"
#include "lib/scheduler.h"
#include "lib/network_check.h"
#include "services/device_info.h"
#include "services/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Paths to LED triggers
#define GREEN_LED_TRIGGER "/sys/devices/platform/leds/leds/green:lan/trigger"
#define RED_LED_TRIGGER "/sys/devices/platform/leds/leds/red:wan/trigger"
#define BLUE_LED_TRIGGER "/sys/devices/platform/leds/leds/blue:wlan2g/trigger"

typedef struct {
    DeviceInfo *device_info;
    Scheduler *scheduler;
} DiagnosticTaskContext;

// Write to LED trigger
static void set_led_trigger(const char *led_path, const char *mode) {
    FILE *fp = fopen(led_path, "w");
    if (fp) {
        fprintf(fp, "%s", mode);
        fclose(fp);
        console(CONSOLE_DEBUG, "Set LED at '%s' to mode '%s'", led_path, mode);
    } else {
        console(CONSOLE_ERROR, "Failed to write to LED at '%s' with mode '%s'", led_path, mode);
    }
}

// Initialize LED states
void init_diagnostic_service(void) {
    console(CONSOLE_DEBUG, "Initializing diagnostic service");
    set_led_trigger(RED_LED_TRIGGER, "timer");
    set_led_trigger(BLUE_LED_TRIGGER, "timer");
    set_led_trigger(GREEN_LED_TRIGGER, "none");
}

// Update LED status based on internet connectivity
void update_led_status(bool internet_connected) {
    if (internet_connected) {
        console(CONSOLE_INFO, "Internet is connected. Setting LED to indicate connectivity.");
        set_led_trigger(GREEN_LED_TRIGGER, "default-one"); // Solid green
        set_led_trigger(RED_LED_TRIGGER, "none");
        set_led_trigger(BLUE_LED_TRIGGER, "none");
    } else {
        console(CONSOLE_INFO, "No internet connectivity. Setting LED to indicate disconnection.");
        set_led_trigger(GREEN_LED_TRIGGER, "none");
        set_led_trigger(RED_LED_TRIGGER, "timer"); // Blinking red
        set_led_trigger(BLUE_LED_TRIGGER, "none");
    }
}

// Diagnostic task to check internet and update LED status
void diagnostic_task(Scheduler *sch, void *task_context) {
    DiagnosticTaskContext *context = (DiagnosticTaskContext *)task_context;

    // Verify if the device is "Genesis"
    if (strcmp(context->device_info->name, "Genesis") != 0) {
        console(CONSOLE_DEBUG, "Device is not Genesis. Skipping diagnostic task.");
        return;
    }

    console(CONSOLE_DEBUG, "Running diagnostic task for Genesis");

    // Check internet connectivity
    bool internet_status = internet_check();

    // Update LED status
    update_led_status(internet_status);

    // Reschedule the task for 10 minutes later
    console(CONSOLE_DEBUG, "Rescheduling diagnostic task for 10 minutes later");
    schedule_task(sch, time(NULL) + config.diagnostic_interval, diagnostic_task, "diagnostic_task", context);
}

// Start diagnostic service
void start_diagnostic_service(Scheduler *scheduler, DeviceInfo *device_info) {
    if (strcmp(device_info->name, "Genesis") != 0) {
        console(CONSOLE_DEBUG, "Device is not Genesis. Diagnostic service will not be started.");
        return;
    }

    DiagnosticTaskContext *context = (DiagnosticTaskContext *)malloc(sizeof(DiagnosticTaskContext));
    if (context == NULL) {
        console(CONSOLE_ERROR, "Failed to allocate memory for diagnostic task context");
        return;
    }

    context->scheduler = scheduler;
    context->device_info = device_info;

    console(CONSOLE_DEBUG, "Scheduling diagnostic service for Genesis");

    // Schedule the first execution of the diagnostic task
    diagnostic_task(scheduler, context);
}