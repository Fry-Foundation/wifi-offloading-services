#ifndef UBUS_H
#define UBUS_H

#include <libubus.h>
#include <stdbool.h>
#include <time.h>

/**
 * UBUS Token Management Functions
 * These functions handle communication with wayru-agent for access tokens
 */

/**
 * Get access token from wayru-agent via UBUS (synchronous)
 * @param token_buf Buffer to store the token (should be at least 256 bytes)
 * @param token_size Size of the token buffer  
 * @param expiry Pointer to store token expiry time
 * @return 0 on success, negative error code on failure
 */
int ubus_get_access_token_sync(char *token_buf, size_t token_size, time_t *expiry);

/**
 * Check if UBUS is available for token requests
 * @return true if UBUS can be used and wayru-agent is available, false otherwise
 */
bool ubus_is_available_for_tokens(void);

#endif // UBUS_H