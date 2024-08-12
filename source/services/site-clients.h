#ifndef SITE_CLIENTS_H
#define SITE_CLIENTS_H

#include "mosquitto.h"

void site_clients_service(struct mosquitto *mosq, char *site);

#endif // SITE_CLIENTS_H
