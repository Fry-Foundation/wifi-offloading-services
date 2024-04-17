#ifndef CONFIG_H
#define CONFIG_H

#define DEFAULT_CONFIGFILE "/etc/wayru-os-services/config.uci"
#define DEFAULT_ENABLED "1"
#define DEFAULT_MAIN_API "https://api.wayru.tech"
#define DEFAULT_ACCOUNTING_ENABLED "1"
#define DEFAULT_ACCOUNTING_INTERVAL "300"
#define DEFAULT_ACCOUNTING_API "https://wifi.api.wayru.tech"
#define DEFAULT_ACCESS_TASK_INTERVAL "120"


typedef struct
{
    int devEnv;
    char *basePath;
    char *deviceId;
    char *mac;
    char *name;
    char *brand;
    char *model;
    char *public_ip;
    char *os_name;
    char *osVersion;
    char *servicesVersion;
    int enabled;
    char *main_api;
    int accounting_enabled;
    int accounting_interval;
    char *accounting_api;
    int access_task_interval;
} Config;

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
    int access_task_interval);

void cleanConfig();

Config getConfig();

#endif // CONFIG_H
