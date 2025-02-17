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
#define BLUE_LED_TRIGGER_ODYSSEY "/sys/devices/platform/leds/leds/blue:wlan/trigger"

static Console csl = {
    .topic = "diagnostic",
};

typedef struct {
    AccessToken *access_token;
} DiagnosticTaskContext;

static DeviceInfo *diagnostic_device_info;

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
void init_diagnostic_service(DeviceInfo *device_info) {
    print_debug(&csl, "Initializing LED diagnostic service");
    diagnostic_device_info = device_info;
}

// Update LED status based on internet connectivity
void update_led_status(bool ok, const char *context) {
    if (strcmp(diagnostic_device_info->name, "Genesis") == 0 || strcmp(diagnostic_device_info->name, "Odyssey") == 0) {
        print_info(&csl, "Updating LEDs for device: %s", diagnostic_device_info->name, context);

        const char *blue_led = strcmp(diagnostic_device_info->name, "Odyssey") == 0 ? BLUE_LED_TRIGGER_ODYSSEY : BLUE_LED_TRIGGER;

        //print_info(&csl, "Device is Genesis. Updating LEDs. Context: %s", context);
        if (ok) {
            print_info(&csl, "Setting LED to indicate connectivity. Context: %s", context);
            set_led_trigger(GREEN_LED_TRIGGER, "default-on"); // Solid green
            set_led_trigger(RED_LED_TRIGGER, "none");
            set_led_trigger(blue_led, "none");
        } else {
            print_info(&csl, "Setting LED to indicate disconnection. Context: %s", context);
            set_led_trigger(GREEN_LED_TRIGGER, "none");
            set_led_trigger(RED_LED_TRIGGER, "timer"); // Blinking red
            set_led_trigger(blue_led, "none");
        }
    }
}

// Diagnostic task to check internet and update LED status
void diagnostic_task(Scheduler *sch, void *task_context) {
    print_info(&csl, "Running diagnostic task");

    // Check internet status
    bool internet_status = internet_check();
    //update_led_status(internet_status, "Internet check - Diagnostic task");
    print_info(&csl, "Diagnostic internet status: %s", internet_status ? "connected" : "disconnected");
    if (!internet_status) {
        print_error(&csl, "No internet connection. Requesting exit.");
        request_cleanup_and_exit();
        return;
    }

    // Check valid token
    DiagnosticTaskContext *context = (DiagnosticTaskContext *)task_context;
    if (!is_token_valid(context->access_token)) {
        print_error(&csl, "Access token is invalid. Requesting exit.");
        request_cleanup_and_exit();
        return;
    }

    // Reschedule the task for 10 minutes later
    print_debug(&csl, "Rescheduling diagnostic task for 2 minutes later");
    schedule_task(sch, time(NULL) + config.diagnostic_interval, diagnostic_task, "diagnostic_task", context);
}

// Start diagnostic service
void start_diagnostic_service(Scheduler *scheduler, AccessToken *access_token) {
    DiagnosticTaskContext *context = (DiagnosticTaskContext *)malloc(sizeof(DiagnosticTaskContext));
    if (context == NULL) {
        print_error(&csl, "Failed to allocate memory for diagnostic task context");
        return;
    }

    context->access_token = access_token;

    print_debug(&csl, "Scheduling diagnostic service");

    // Schedule the first execution of the diagnostic task
    diagnostic_task(scheduler, context);
}
