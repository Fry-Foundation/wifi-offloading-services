#!/bin/sh

start_session() {
  logger -t "BINAUTH" "Starting session for $client_mac with token $client_token"

  custom_escaped=$(printf "${custom//%/\\x}")

  data="{ \"client_mac\": \"$client_mac\", \"bytes_incoming\": \"$bytes_incoming\", \"bytes_outgoing\": \"$bytes_outgoing\", \"session_start\": \"$session_start\", \"session_end\": \"$session_end\", \"client_token\": \"$client_token\", \"custom\": \"$custom_escaped\" }"

  curl --header "Content-Type: application/json" \
  --request POST \
  --data "$data" \
  https://api.internal.wayru.net/wifi-sessions
}

end_session() {
  logger -t "BINAUTH" "Ending session for $client_mac with token $client_token"

  data="{ \"client_mac\": \"$client_mac\", \"bytes_incoming\": \"$bytes_incoming\", \"bytes_outgoing\": \"$bytes_outgoing\", \"session_start\": \"$session_start\", \"session_end\": \"$session_end\", \"client_token\": \"$client_token\", \"end_reason\": \"$deauth_reason\" }"

  curl --header "Content-Type: application/json" \
  --request POST \
  --data "$data" \
  https://api.internal.wayru.net/wifi-sessions/end
}

#
# Get the action method from NDS ie the first command line argument.
#
# Possible values are:
# "auth_client" - NDS requests validation of the client (legacy - deprecated)
# "client_auth" - NDS has authorised the client (legacy - deprecated)
# "client_deauth" - NDS has deauthenticated the client on request (logout)
# "idle_deauth" - NDS has deauthenticated the client because the idle timeout duration has been exceeded
# "timeout_deauth" - NDS has deauthenticated the client because the session length duration has been exceeded
# "downquota_deauth" - NDS has deauthenticated the client because the client's download quota has been exceeded
# "upquota_deauth" - NDS has deauthenticated the client because the client's upload quota has been exceeded
# "ndsctl_auth" - NDS has authorised the client because of an ndsctl command
# "ndsctl_deauth" - NDS has deauthenticated the client because of an ndsctl command
# "shutdown_deauth" - NDS has deauthenticated the client because it received a shutdown command
#

action=$1
logger -t BINAUTH "action is $action"

client_mac=$2
bytes_incoming=$3
bytes_outgoing=$4
session_start=$5
session_end=$6
client_token=$7

if [ $action = "ndsctl_auth" ]; then
  custom=$8

  start_session
elif [ $action = "ndsctl_deauth" ]; then
  deauth_reason="nds"
  end_session
elif [ $action = "client_deauth" ]; then
  deauth_reason="client"
  end_session
elif [ $action = "idle_deauth" ]; then
  deauth_reason="idle"
  end_session
 elif [ $action = "timeout_deauth" ]; then
  deauth_reason="timeout"
  end_session
elif [ $action = "downquota_deauth" ]; then
  deauth_reason="download_quota"
  end_session
elif [ $action = "upquota_deauth" ]; then
  deauth_reason="upload_quota"
  end_session  
elif [ $action = "shutdown_deauth" ]; then
  deauth_reason="shutdown"
  end_session       
else 
  echo "$action is not yet handled"
fi
