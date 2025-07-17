#ifndef RENDERER_H
#define RENDERER_H

#include <stdbool.h>

/**
 * Applies configuration from JSON using ucode renderer
 * @param json_config The JSON configuration string to apply
 * @param dev_mode Whether to run in development mode
 * @return 0 on success, non-zero on failure
 */
int apply_config(const char *json_config, bool dev_mode);

/**
 * Applies configuration from JSON WITHOUT restarting services
 * @param json_config The JSON configuration string to apply
 * @param dev_mode Whether to run in development mode
 * @return 0 on success, non-zero on failure
 */
int apply_config_without_restarts(const char *json_config, bool dev_mode);

/**
 * Check if configuration affects wayru-config itself
 * @param json_config The JSON configuration string to check
 * @return true if wayru-config needs restart, false otherwise
 */
bool config_affects_wayru_config(const char *json_config, bool dev_mode);

/**
 * Check if configuration affects wireless settings
 * @param json_config The JSON configuration string to check
 * @return true if wireless config changed, false otherwise
 */
bool config_affects_wireless(const char *json_config, bool dev_mode);

/**
 * Check if configuration affects wayru-agent
 * @param json_config The JSON configuration string to check
 * @return true if wayru-agent config changed, false otherwise
 */
bool config_affects_wayru_agent(const char *json_config, bool dev_mode);

/**
 * Check if configuration affects wayru-collector
 * @param json_config The JSON configuration string to check
 * @return true if wayru-collector config changed, false otherwise
 */
bool config_affects_wayru_collector(const char *json_config, bool dev_mode);

bool config_affects_opennds(const char *json_config, bool dev_mode);

/**
 * Set the development mode for the renderer
 * @param dev_mode True to enable development mode, false to disable
 */
void set_renderer_dev_mode(bool dev_mode);

/**
 * Reset the configuration section hashes
 */
void reset_config_section_hashes(void);

/**
 * Clear all section hashes
 * @param dev_mode Whether to run in development mode
 */
void clear_all_section_hashes(bool dev_mode);

#endif /* RENDERER_H */