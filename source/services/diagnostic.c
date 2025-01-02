#include "diagnostic.h"
#include "lib/console.h"
#include "lib/scheduler.h"
#include "lib/network_check.h"
#include "services/device_info.h"
#include "services/config.h"
#include "services/access_token.h"
#include "services/exit_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Paths to LED triggers
#define GREEN_LED_TRIGGER "/sys/devices/platform/leds/leds/green:lan/trigger"
#define RED_LED_TRIGGER "/sys/devices/platform/leds/leds/red:wan/trigger"
#define BLUE_LED_TRIGGER "/sys/devices/platform/leds/leds/blue:wlan2g/trigger"

static Console csl = {
    .topic = "diagnostic",
};

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
        print_debug(&csl, "Set LED at '%s' to mode '%s'", led_path, mode);

    } else {
        print_error(&csl, "Failed to write to LED at '%s' with mode '%s'", led_path, mode);
    }
}

// Initialize LED states
void init_diagnostic_service(void) {
    print_debug(&csl, "Initializing LED diagnostic service");
    set_led_trigger(RED_LED_TRIGGER, "timer");
    set_led_trigger(BLUE_LED_TRIGGER, "timer");
    set_led_trigger(GREEN_LED_TRIGGER, "none");
}

// Update LED status based on internet connectivity
void update_led_status(bool internet_connected) {
    if (internet_connected) {
        print_info(&csl, "Internet is connected. Setting LED to indicate connectivity.");
        set_led_trigger(GREEN_LED_TRIGGER, "default-on"); // Solid green
        set_led_trigger(RED_LED_TRIGGER, "none");
        set_led_trigger(BLUE_LED_TRIGGER, "none");
    } else {
        print_info(&csl, "No internet connectivity. Setting LED to indicate disconnection.");
        set_led_trigger(GREEN_LED_TRIGGER, "none");
        set_led_trigger(RED_LED_TRIGGER, "timer"); // Blinking red
        set_led_trigger(BLUE_LED_TRIGGER, "none");
    }
}

// Diagnostic task to check internet and update LED status
void diagnostic_task(Scheduler *sch, void *task_context) {
    DiagnosticTaskContext *context = (DiagnosticTaskContext *)task_context;

    print_info(&csl, "Running diagnostic task");

    // Run LED update only if device is Genesis
    if (strcmp(context->device_info->name, "Genesis") == 0) {
        print_info(&csl, "Device is Genesis. Running LED diagnostic task.");
        bool internet_status = internet_check();
        print_info(&csl, "Diagnostic task for led, internet status: %s", internet_status ? "connected" : "disconnected");
        update_led_status(internet_status);
    }    

    // Check broker connection

    // Check valid token
    validate_or_exit();
    
    // Reschedule the task for 10 minutes later
    print_debug(&csl, "Rescheduling diagnostic task for 2 minutes later");
    schedule_task(sch, time(NULL) + config.diagnostic_interval, diagnostic_task, "diagnostic_task", context);
}

// Start diagnostic service
void start_diagnostic_service(Scheduler *scheduler, DeviceInfo *device_info) {

    DiagnosticTaskContext *context = (DiagnosticTaskContext *)malloc(sizeof(DiagnosticTaskContext));
    if (context == NULL) {
        print_error(&csl, "Failed to allocate memory for diagnostic task context");
        return;
    }

    context->scheduler = scheduler;
    context->device_info = device_info;

    print_debug(&csl, "Scheduling diagnostic service");

    // Schedule the first execution of the diagnostic task
    diagnostic_task(scheduler, context);
}
