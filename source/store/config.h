#ifndef CONFIG_H
#define CONFIG_H

#define DEFAULT_CONFIGFILE "/etc/wayru-os-services/config.uci"
#define DEFAULT_ENABLED "1"
#define DEFAULT_MAIN_API "https://api.wayru.tech"
#define DEFAULT_ACCOUNTING_API "https://api.wifi.wayru.tech"

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
    // char configfile[255];
    // int enabled;
    // char *main_api;
    // char *accounting_api;
} Config;

/*typedef struct
{
    char configfile[255];
    int enabled;
    char *main_api
    char *accounting_api;
} s_config;*/

// s_config *config_get_config(void);*/

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
    char *servicesVersion);
// char configfile[255],
// int enabled,
// char *main_api,
// char *accounting_api);

void cleanConfig();

void set_default_values();

Config getConfig();

#endif // CONFIG_H
