#include <stdlib.h>
#include <string.h>
#include "config.h"

Config config;

void initConfig(
    int devEnv,
    char *basePath,
    char *deviceId,
    char *mac,
    char *brand,
    char *model,
    char *osVersion,
    char *servicesVersion)
{
    config.devEnv = devEnv;
    config.basePath = strdup(basePath);
    config.deviceId = strdup(deviceId);
    config.mac = strdup(mac);
    config.brand = strdup(brand);
    config.model = strdup(model);
    config.osVersion = strdup(osVersion);
    config.servicesVersion = strdup(servicesVersion);
}

void cleanConfig()
{
    free(config.basePath);
    free(config.deviceId);
    free(config.mac);
    free(config.brand);
    free(config.model);
    free(config.osVersion);
    free(config.servicesVersion);
}

Config getConfig()
{
    return config;
}