#ifndef SITE_CLIENTS_H
#define SITE_CLIENTS_H

#include "mosquitto.h"
#include "services/device-context.h"

void site_clients_service(struct mosquitto *mosq, Site *site);

#endif // SITE_CLIENTS_H
