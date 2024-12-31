#include "site-clients.h"
#include "lib/console.h"
#include "lib/scheduler.h"
#include "lib/script_runner.h"
#include "mosquitto.h"
#include "services/config.h"
#include "services/device-context.h"
#include "services/mqtt.h"
#include <errno.h>
#include <fcntl.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define SET_BINAUTH_SCRIPT "nds-set-binauth.lua"
#define BINAUTH_SCRIPT "nds-binauth.sh"
#define SITE_CLIENTS_FIFO "site-clients-fifo"
#define SESSION_TIMEOUT "60"
#define UPLOAD_RATE "0"
#define DOWNLOAD_RATE "0"
#define UPLOAD_QUOTA "0"
#define DOWNLOAD_QUOTA "0"
#define CUSTOM "custom_placeholder"

static Console csl = {
    .topic = "site-clients",
};

typedef struct {
    struct mosquitto *mosq;
    Site *site;
    int fifo_fd;
} SiteClientsTaskContext;

void handle_connect(struct json_object *parsed_json) {
    struct json_object *mac;
    struct json_object *sessiontimeout;
    struct json_object *uploadrate;
    struct json_object *downloadrate;
    struct json_object *uploadquota;
    struct json_object *downloadquota;
    struct json_object *custom;

    // Extract the expected values from the JSON object
    json_object_object_get_ex(parsed_json, "mac", &mac);
    json_object_object_get_ex(parsed_json, "sessiontimeout", &sessiontimeout);
    json_object_object_get_ex(parsed_json, "uploadrate", &uploadrate);
    json_object_object_get_ex(parsed_json, "downloadrate", &downloadrate);
    json_object_object_get_ex(parsed_json, "uploadquota", &uploadquota);
    json_object_object_get_ex(parsed_json, "downloadquota", &downloadquota);
    json_object_object_get_ex(parsed_json, "custom", &custom);

    // Build the command
    char command[512];
    snprintf(command, sizeof(command), "%s/%s add %s %s %s %s %s %s %s", config.scripts_path,
             "nds-set-preemptive-list.lua", json_object_get_string(mac), json_object_get_string(sessiontimeout),
             json_object_get_string(uploadrate), json_object_get_string(downloadrate),
             json_object_get_string(uploadquota), json_object_get_string(downloadquota),
             json_object_get_string(custom));

    print_debug(&csl, "Command: %s", command);

    // Run the script
    char *output = run_script(command);
    print_debug(&csl, "Script output: %s", output);

    // Clean up
    free(output);
}

void handle_disconnect(struct json_object *parsed_json) {
    struct json_object *mac;

    // Extract values from the JSON object
    json_object_object_get_ex(parsed_json, "mac", &mac);

    // Build the command
    char command[512];
    snprintf(command, sizeof(command), "%s/%s remove %s", config.scripts_path, "nds-set-preemptive-list.lua",
             json_object_get_string(mac));

    // Run the script
    char *output = run_script(command);
    print_debug(&csl, "Script output: %s", output);

    // Clean up
    free(output);
}

void site_clients_callback(struct mosquitto *mosq, const struct mosquitto_message *message) {
    print_debug(&csl, "Received message on site clients topic, payload: %s", (char *)message->payload);

    // Parse the JSON payload
    struct json_object *parsed_json;
    struct json_object *type;

    parsed_json = json_tokener_parse((char *)message->payload);
    if (!parsed_json) {
        print_error(&csl, "Failed to parse clients topic payload JSON");
        return;
    }

    if (!json_object_object_get_ex(parsed_json, "type", &type)) {
        print_error(&csl, "Failed to extract type field from clients topic payload JSON");
        json_object_put(parsed_json);
        return;
    }

    const char *type_str = json_object_get_string(type);

    // Handle the message based on the type
    if (strcmp(type_str, "connect") == 0) {
        handle_connect(parsed_json);
    } else if (strcmp(type_str, "disconnect") == 0) {
        handle_disconnect(parsed_json);
    } else {
        print_error(&csl, "Unknown clients topic type: %s", type_str);
    }

    // Clean up
    json_object_put(parsed_json);
}

void configure_site_mac(char *mac) {
    // Build the command
    char command[512];
    snprintf(command, sizeof(command), "%s/%s %s", config.scripts_path, "network-set-mac.lua", mac);

    // Run the script
    char *output = run_script(command);
    print_debug(&csl, "Script output: %s", output);

    // Clean up
    free(output);
}

void configure_binauth() {
    // Build the command
    char binauth_script_path[256];
    snprintf(binauth_script_path, sizeof(binauth_script_path), "%s/%s", config.scripts_path, BINAUTH_SCRIPT);

    char command[512];
    snprintf(command, sizeof(command), "%s/%s %s", config.scripts_path, SET_BINAUTH_SCRIPT, binauth_script_path);

    // Run the script
    char *output = run_script(command);
    print_debug(&csl, "Script output: %s", output);

    // Clean up
    free(output);
}

void site_clients_task(Scheduler *sch, void *task_context) {
    SiteClientsTaskContext *context = (SiteClientsTaskContext *)task_context;
    char buffer[512];

    // Read all available data from the FIFO
    ssize_t bytesRead = read(context->fifo_fd, buffer, sizeof(buffer) - 1);
    print_debug(&csl, "Read %ld bytes from fifo", bytesRead);
    if (bytesRead > 0) {
        // Null-terminate the buffer to make it a valid C-string
        buffer[bytesRead] = '\0';

        // Process the data from FIFO
        print_debug(&csl, "Received from fifo: %s", buffer);

        // Split the buffer into lines
        char *line = strtok(buffer, "\n");
        print_debug(&csl, "Line after strtok: %s", line);

        while (line != NULL) {
            char event_type[64];
            char mac[64];

            // Use sscanf to extract the two words from the line
            int ret = sscanf(line, "%s %s", event_type, mac);

            // Display the results
            print_debug(&csl, "Event type: %s, MAC: %s", event_type, mac);

            if (ret == 2) {
                if (strcmp(event_type, "connect") == 0) {
                    // Publish connect event to the broker
                    struct json_object *json_payload = json_object_new_object();

                    json_object_object_add(json_payload, "type", json_object_new_string("connect"));
                    json_object_object_add(json_payload, "mac", json_object_new_string(mac));

                    // Note that we use default values for the other fields
                    // ... a more robust approach would find the client in the database
                    // ... and use the actual values of the original session
                    json_object_object_add(json_payload, "sessiontimeout", json_object_new_string(SESSION_TIMEOUT));
                    json_object_object_add(json_payload, "uploadrate", json_object_new_string(UPLOAD_RATE));
                    json_object_object_add(json_payload, "downloadrate", json_object_new_string(DOWNLOAD_RATE));
                    json_object_object_add(json_payload, "uploadquota", json_object_new_string(UPLOAD_QUOTA));
                    json_object_object_add(json_payload, "downloadquota", json_object_new_string(DOWNLOAD_QUOTA));
                    json_object_object_add(json_payload, "custom", json_object_new_string(CUSTOM));

                    const char *payload_str = json_object_to_json_string(json_payload);

                    char topic[256];
                    snprintf(topic, sizeof(topic), "site/%s/clients", context->site->id);

                    print_debug(&csl, "Publishing to topic: %s, payload: %s", topic, payload_str);

                    publish_mqtt(context->mosq, topic, payload_str, 0);

                    json_object_put(json_payload);
                } else if (strcmp(event_type, "disconnect") == 0) {
                    struct json_object *json_payload = json_object_new_object();

                    json_object_object_add(json_payload, "type", json_object_new_string("disconnect"));
                    json_object_object_add(json_payload, "mac", json_object_new_string(mac));

                    const char *payload_str = json_object_to_json_string(json_payload);

                    char topic[256];
                    snprintf(topic, sizeof(topic), "site/%s/clients", context->site->id);

                    print_debug(&csl, "Publishing to topic: %s, payload: %s", topic, payload_str);

                    publish_mqtt(context->mosq, topic, payload_str, 0);

                    json_object_put(json_payload);
                } else {
                    print_error(&csl, "Unknown event type: %s", event_type);
                }
            } else {
                print_error(&csl, "Invalid line: %s", line);
            }

            // Get next line
            line = strtok(NULL, "\n");
        }
    } else if (bytesRead == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        // Handle actual error
        print_error(&csl, "Failed to read from site clients fifo");
    }

    // Re-schedule the task to run again after interval
    schedule_task(sch, time(NULL) + config.site_clients_interval, site_clients_task, "site clients", context);
}

int init_site_clients_fifo() {
    // Create the dir within the /tmp folder and FIFO file if it doesn't already exist
    struct stat st = {0};
    char fifo_dir[256];
    char fifo_file[256];

    if (config.dev_env) {
        snprintf(fifo_dir, sizeof(fifo_dir), "./tmp");
        snprintf(fifo_file, sizeof(fifo_file), "%s/%s", "./tmp", SITE_CLIENTS_FIFO);
    } else {
        snprintf(fifo_dir, sizeof(fifo_dir), "/tmp/wayru-os-services");
        snprintf(fifo_file, sizeof(fifo_file), "%s/%s", "/tmp/wayru-os-services", SITE_CLIENTS_FIFO);
    }

    // Check if directory exists
    if (stat(fifo_dir, &st) == -1) {
        // Directory does not exist, create it
        if (mkdir(fifo_dir, 0700) == -1) {
            print_error(&csl, "failed to create site clients fifo directory");
            return -1;
        }
        print_debug(&csl, "directory created: %s", fifo_dir);
    } else {
        print_debug(&csl, "directory already exists: %s", fifo_dir);
    }

    // Create the FIFO file if it doesn't exist
    if (mkfifo(fifo_file, 0666) == -1) {
        if (errno != EEXIST) {
            print_error(&csl, "failed to create site clients fifo");
            return -1;
        }
    }

    int fifo_fd = open(fifo_file, O_RDONLY | O_NONBLOCK);
    if (fifo_fd == -1) {
        print_error(&csl, "failed to open site clients fifo");
        return -1;
    }

    print_info(&csl, "site clients fifo opened, fifo_fd: %d", fifo_fd);

    return fifo_fd;
}

void site_clients_service(Scheduler *sch, struct mosquitto *mosq, int site_fifo_fd, Site *site) {
    if (site == NULL || site->id == NULL || site->mac == NULL) {
        print_info(&csl, "no site to subscribe to or incomplete details");
        return;
    }

    SiteClientsTaskContext *context = (SiteClientsTaskContext *)malloc(sizeof(SiteClientsTaskContext));
    if (context == NULL) {
        print_error(&csl, "failed to allocate memory for site clients task context");
        return;
    }

    context->mosq = mosq;
    context->site = site;
    context->fifo_fd = site_fifo_fd;

    configure_binauth();
    configure_site_mac(site->mac);

    char site_clients_topic[256];
    snprintf(site_clients_topic, sizeof(site_clients_topic), "site/%s/clients", site->id);
    subscribe_mqtt(mosq, site_clients_topic, 1, site_clients_callback);

    site_clients_task(sch, context);
}

void clean_site_clients_fifo(int *site_fifo_fd) {
    if (site_fifo_fd == NULL) {
        print_error(&csl, "site clients fifo fd is NULL");
        return;
    }

    // Close the FIFO file descriptor if it is valid (non-negative)
    if (*site_fifo_fd >= 0) {
        if (close(*site_fifo_fd) == 0) {
            print_info(&csl, "site clients fifo closed, fifo_fd: %d", *site_fifo_fd);
        } else {
            print_error(&csl, "failed to close site clients fifo, fifo_fd: %d", *site_fifo_fd);
        }
        *site_fifo_fd = -1;
    } else {
        print_warn(&csl, "site clients fifo already closed or invalid, fifo_fd: %d", *site_fifo_fd);
    }

    // Build the FIFO file path and unlink it
    char fifo_path[256];
    if (snprintf(fifo_path, sizeof(fifo_path), "%s/wayru-os-services/%s", config.temp_path, SITE_CLIENTS_FIFO) >= (int)sizeof(fifo_path)) {
        print_error(&csl, "site clients fifo path exceeds buffer size");
        return;
    }

    if (unlink(fifo_path) == 0) {
        print_info(&csl, "site clients fifo unlinked, path: %s", fifo_path);
    } else {
        print_error(&csl, "failed to unlink site clients fifo, path: %s", fifo_path);
    }
    unlink(fifo_path);
}
