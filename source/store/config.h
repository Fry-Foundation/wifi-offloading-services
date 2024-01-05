#ifndef CONFIG_H
#define CONFIG_H

#define DEFAULT_CONFIGFILE "/etc/wayru-os-services/config.uci"
#define DEFAULT_ENABLED "1"
#define DEFAULT_MAIN_API "https://api.wayru.tech"
#define DEFAULT_ACCOUNTING_API "https://wifi.api.wayru.tech"

typedef struct
{
    int devEnv;
    char *basePath;
    char *deviceId;
    char *mac;
    char *brand;
    char *model;
    char *public_ip;
    char *os_name;
    char *osVersion;
    char *servicesVersion;
    int enabled;
    char *main_api;
    char *accounting_api;
} Config;

void initConfig(
    int devEnv,
    char *basePath,
    char *deviceId,
    char *mac,
    char *brand,
    char *model,
    char *public_ip,
    char *os_name,
    char *osVersion,
    char *servicesVersion,
    int enabled,
    char *main_api,
    char *accounting_api);

void cleanConfig();

Config getConfig();

#endif // CONFIG_H
