#ifndef UCI_PARSER_H
#define UCI_PARSER_H

#include <stdbool.h>
#include "config.h"

/**
 * Parse UCI configuration file and populate the provided config structure
 * @param config_path Path to the UCI configuration file
 * @param config Pointer to the config structure to populate
 * @return true if parsing was successful, false otherwise
 */
bool parse_uci_config(const char* config_path, Config* config);

#endif // UCI_PARSER_H 