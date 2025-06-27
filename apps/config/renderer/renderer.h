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

#endif /* RENDERER_H */