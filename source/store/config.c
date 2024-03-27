#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "config.h"

Config config;

void initConfig(
    int devEnv,
    char *basePath,
    char *deviceId,
    char *mac,
    char *name,
    char *brand,
    char *model,
    char *public_ip,
    char *os_name,
    char *osVersion,
    char *servicesVersion,
    int enabled,
    char *main_api,
    int accounting_enabled,
    int accounting_interval,
    char *accounting_api,
    int access_task_interval)
{
    config.devEnv = devEnv;
    config.basePath = strdup(basePath);
    config.deviceId = strdup(deviceId);
    config.mac = strdup(mac);
    config.name = strdup(name);
    config.brand = strdup(brand);
    config.model = strdup(model);
    config.public_ip = strdup(public_ip);
    config.os_name = strdup(os_name);
    config.osVersion = strdup(osVersion);
    config.servicesVersion = strdup(servicesVersion);
    config.enabled = enabled;
    config.main_api = strdup(main_api);
    config.accounting_enabled = accounting_enabled;
    config.accounting_interval = accounting_interval;
    config.accounting_api = strdup(accounting_api);
    config.access_task_interval = access_task_interval;
}

void cleanConfig()
{
    free(config.basePath);
    free(config.deviceId);
    free(config.mac);
    free(config.name);
    free(config.brand);
    free(config.model);
    free(config.public_ip);
    free(config.os_name);
    free(config.osVersion);
    free(config.servicesVersion);
    free(config.main_api);
    free(config.accounting_api);
}

Config getConfig()
{
    return config;
}