#!/bin/sh

FIFO_PATH="../tmp/site-clients-fifo"

#
# Get the action method from NDS ie the first command line argument.
#
# Possible values are:
# "auth_client" - NDS requests validation of the client
# "client_auth" - NDS has authorised the client
# "client_deauth" - NDS has deauthenticated the client on request (logout)
# "idle_deauth" - NDS has deauthenticated the client because the idle timeout duration has been exceeded
# "timeout_deauth" - NDS has deauthenticated the client because the session length duration has been exceeded
# "downquota_deauth" - NDS has deauthenticated the client because the client's download quota has been exceeded
# "upquota_deauth" - NDS has deauthenticated the client because the client's upload quota has been exceeded
# "ndsctl_auth" - NDS has authorised the client because of an ndsctl command
# "ndsctl_deauth" - NDS has deauthenticated the client because of an ndsctl command
# "shutdown_deauth" - NDS has deauthenticated the client because it received a shutdown command
#
method=$1
mac=$2

if [ "$method" = "client_deauth" ] || [ "$method" = "idle_deauth" ] || [ "$method" = "timeout_deauth" ] || \
   [ "$method" = "downquota_deauth" ] || [ "$method" = "upquota_deauth" ] || \
   [ "$method" = "ndsctl_deauth" ] || [ "$method" = "shutdown_deauth" ]; then
    echo "disconnect $mac" > "$FIFO_PATH" &
else
    echo "connect $mac" > "$FIFO_PATH" &
fi

exit 0
