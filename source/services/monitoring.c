#include "lib/console.h"
#include "lib/scheduler.h"
#include "lib/script_runner.h"
#include "services/config.h"
#include "services/device_info.h"
#include "services/mqtt.h"
#include <json-c/json.h>
#include <lib/console.h>
#include <mosquitto.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static struct mosquitto *mosq;
static DeviceInfo *device_info;

typedef struct {
    int wifi_clients;
    unsigned long memory_total;
    unsigned long memory_free;
    unsigned long memory_used;
    unsigned long memory_shared;
    unsigned long memory_buffered;
    int cpu_count;
    float cpu_load;
    int cpu_load_percent;
    unsigned long disk_used;
    unsigned long disk_size;
    unsigned long disk_available;
    int disk_used_percent;
    int radio_count;
    int radio_live;
} DeviceData;

void parse_output(const char *output, DeviceData *info) {
    struct {
        const char *key;
        const char *format;
        void *value;
    } mappings[] = {
        {"wifi_clients:", " %d", &info->wifi_clients},
        {"memory_total:", " %lu", &info->memory_total},
        {"memory_free:", " %lu", &info->memory_free},
        {"memory_used:", " %lu", &info->memory_used},
        {"memory_shared:", " %lu", &info->memory_shared},
        {"memory_buffered:", " %lu", &info->memory_buffered},
        {"cpu_count:", " %d", &info->cpu_count},
        {"cpu_load:", " %f", &info->cpu_load},
        {"cpu_load_percent:", " %d", &info->cpu_load_percent},
        {"disk_used:", " %lu", &info->disk_used},
        {"disk_size:", " %lu", &info->disk_size},
        {"disk_available:", " %lu", &info->disk_available},
        {"disk_used_percent:", " %d", &info->disk_used_percent},
        {"radio_count:", " %d", &info->radio_count},
        {"radio_live:", " %d", &info->radio_live},
    };

    const int mappings_count = sizeof(mappings) / sizeof(mappings[0]);
    char *line = strtok(output, "\n");
    while (line != NULL) {
        for (int i = 0; i < mappings_count; ++i) {
            if (strstr(line, mappings[i].key) == line) {
                sscanf(line + strlen(mappings[i].key), mappings[i].format, mappings[i].value);
                break;
            }
        }
        line = strtok(NULL, "\n");
    }
}

json_object *createjson(DeviceData *device_data, json_object *jobj, int timestamp) {
    json_object_object_add(jobj, "device_id", json_object_new_string(device_info->device_id));
    json_object_object_add(jobj, "timestamp", json_object_new_int(timestamp));
    json_object_object_add(jobj, "wifi_clients", json_object_new_int(device_data->wifi_clients));
    json_object_object_add(jobj, "memory_total", json_object_new_int64(device_data->memory_total));
    json_object_object_add(jobj, "memory_free", json_object_new_int64(device_data->memory_free));
    json_object_object_add(jobj, "memory_used", json_object_new_int64(device_data->memory_used));
    json_object_object_add(jobj, "memory_shared", json_object_new_int64(device_data->memory_shared));
    json_object_object_add(jobj, "memory_buffered", json_object_new_int64(device_data->memory_buffered));
    json_object_object_add(jobj, "cpu_count", json_object_new_int(device_data->cpu_count));
    json_object_object_add(jobj, "cpu_load", json_object_new_double(device_data->cpu_load));
    json_object_object_add(jobj, "cpu_load_percent", json_object_new_int(device_data->cpu_load_percent));
    json_object_object_add(jobj, "disk_used", json_object_new_int64(device_data->disk_used));
    json_object_object_add(jobj, "disk_size", json_object_new_int64(device_data->disk_size));
    json_object_object_add(jobj, "disk_available", json_object_new_int64(device_data->disk_available));
    json_object_object_add(jobj, "disk_used_percent", json_object_new_int(device_data->disk_used_percent));
    json_object_object_add(jobj, "radio_count", json_object_new_int(device_data->radio_count));
    json_object_object_add(jobj, "radio_live", json_object_new_int(device_data->radio_live));
    return jobj;
}

void monitoring_task(Scheduler *sch) {
    time_t now;
    time(&now);
    DeviceData device_data;
    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s%s", config.scripts_path, "/retrieve-data.lua");
    char *output = run_script(script_file);
    if (output == NULL) {
        console(CONSOLE_ERROR, "failed to run script %s", script_file);
        return;
    }
    parse_output(output, &device_data);
    free(output);

    json_object *json_device_data = json_object_new_object();
    createjson(&device_data, json_device_data, now);

    const char *device_data_str = json_object_to_json_string(json_device_data);

    console(CONSOLE_INFO, "Device data: %s", device_data_str);
    publish_mqtt(mosq, "monitoring/device-data", device_data_str);

    json_object_put(json_device_data);

    // Schedule monitoring_task to rerun later
    schedule_task(sch, time(NULL) + config.monitoring_interval, monitoring_task, "monitoring");
}

void monitoring_service(Scheduler *sch, struct mosquitto *_mosq, DeviceInfo *_device_info) {
    mosq = _mosq;
    device_info = _device_info;
    monitoring_task(sch);
}
