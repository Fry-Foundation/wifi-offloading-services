#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "config.h"

#define SMALL_BUF 1024

Config config;

// Holds the current configuration
// static s_config uconfig = {{0}};

/** Accessor for the current gateway configuration
return:  A pointer to the current config.  The pointer isn't opaque, but should be treated as READ-ONLY
 */

/*s_config *
config_get_config(void)
{
    return &uconfig;
}

int get_option_from_config(char *msg, int msg_len, const char *option)
{
    char *cmd;

    cmd = calloc(SMALL_BUF);
    snprintf(cmd, SMALL_BUF, "/usr/lib/opennds/libopennds.sh get_option_from_config '%s'", option);

    if (execute_ret_url_encoded(msg, msg_len - 1, cmd) != 0)
    {
        printf("Failed to get option[% s] - retrying ", option);
        sleep(1);

        if (execute_ret_url_encoded(msg, msg_len - 1, cmd) != 0)

        {
            printf("Failed to get option [%s] - giving up", option);
        }
    }

    free(cmd);
    return 0;
}

char *set_option_str(char *option, const char *default_option)
{
    char msg[SMALL_BUF];

    memset(msg, 0, SMALL_BUF);

    get_option_from_config(msg, SMALL_BUF, option);

    if (strcmp(msg, "") == 0)
    {
        return strdup(default_option);
    }
    else
    {
        return strdup(msg);
    }
}
// Sets the default config parameters and initialises the configuration system
void config_init()
{
    // Are we enabled?
    // sscanf(set_option_str("enabled", DEFAULT_ENABLED), "%u", &uconfig.enabled);

    if (uconfig.enabled != 1)
    {
        printf("[init] wayru-os-services is disabled (see \"option enabled\" in config).\n");
        exit(0);
    }
    // parse_commandline
    strncpy(uconfig.configfile, DEFAULT_CONFIGFILE, sizeof(uconfig.configfile) - 1);
    // uconfig.main_api_a = strdup(main_api_a);
    // uconfig.main_api_a = strdup(DEFAULT_MAIN_API_ACCESS);
    uconfig.main_api_a = strdup(DEFAULT_MAIN_API_ACCESS);
}*/

/*void set_default_values(int *enabled, char *main_api, char *accounting_api)
{
    // config.configfile = strdup(DEFAULT_CONFIGFILE);

    if (!config.enabled)
    {
        config.enabled = atoi(DEFAULT_ENABLED);
    }

    if (!config.main_api)
    {
        config.main_api = strdup(DEFAULT_MAIN_API);
    }

    if (!config.accounting_api)
    {
        config.accounting_api = strdup(DEFAULT_ACCOUNTING_API);
    }
}*/

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
    char *servicesVersion)
// char configfile[255],
// int enabled,
// char *main_api,
// char *accounting_api)
{
    config.devEnv = devEnv;
    config.basePath = strdup(basePath);
    config.deviceId = strdup(deviceId);
    config.mac = strdup(mac);
    config.brand = strdup(brand);
    config.model = strdup(model);
    config.public_ip = strdup(public_ip);
    config.os_name = strdup(os_name);
    config.osVersion = strdup(osVersion);
    config.servicesVersion = strdup(servicesVersion);
    // strncpy(config.configfile, configfile, sizeof(config.configfile) - 1);
    // config.enabled = enabled;
    // config.main_api = strdup(main_api);
    // config.accounting_api = strdup(accounting_api);

    /*if (!config.enabled)
    {
        config.enabled = atoi(DEFAULT_ENABLED);
    }

    if (!config.main_api)
    {
        config.main_api = strdup(DEFAULT_MAIN_API);
    }

    if (!config.accounting_api)
    {
        config.accounting_api = strdup(DEFAULT_ACCOUNTING_API);
    }*/
}

void cleanConfig()
{
    free(config.basePath);
    free(config.deviceId);
    free(config.mac);
    free(config.brand);
    free(config.model);
    free(config.public_ip);
    free(config.os_name);
    free(config.osVersion);
    free(config.servicesVersion);
}

Config getConfig()
{
    return config;
}