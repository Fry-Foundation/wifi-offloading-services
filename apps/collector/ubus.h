#ifndef UBUS_H
#define UBUS_H

#include <libubus.h>
#include <stdbool.h>

/**
 * Initialize UBUS connection and subscribe to syslog events
 * @return 0 on success, negative error code on failure
 */
int ubus_init(void);

/**
 * Start the UBUS event loop in a separate thread
 * This function will block and handle UBUS events
 * @return 0 on success, negative error code on failure
 */
int ubus_start_loop(void);

/**
 * Stop the UBUS event loop and cleanup
 */
void ubus_cleanup(void);

/**
 * Check if UBUS connection is active
 * @return true if connected, false otherwise
 */
bool ubus_is_connected(void);

#endif // UBUS_H