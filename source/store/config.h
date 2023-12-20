#ifndef CONFIG_H
#define CONFIG_H

typedef struct
{
    int devEnv;
    char *basePath;
    char *deviceId;
    char *mac;
    char *brand;
    char *model;
    char *public_ip;
    char *osVersion;
    char *servicesVersion;
} Config;

void initConfig(
    int devEnv,
    char *basePath,
    char *deviceId,
    char *mac,
    char *brand,
    char *model,
    char *public_ip,
    char *osVersion,
    char *servicesVersion);

void cleanConfig();

Config getConfig();

#endif // CONFIG_H
