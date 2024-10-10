#ifndef CERT_H
#define CERT_H

#include "services/access_token.h"
#include <stdbool.h>

#define MQTT_CA_FILE_NAME "mqtt-ca.crt"
#define MQTT_KEY_FILE_NAME "mqtt.key"
#define MQTT_CSR_FILE_NAME "mqtt.csr"
#define MQTT_CERT_FILE_NAME "mqtt.crt"

bool attempt_ca_cert(AccessToken *access_token);
bool attempt_generate_and_sign(AccessToken *access_token);

#endif /* CERT_H */
