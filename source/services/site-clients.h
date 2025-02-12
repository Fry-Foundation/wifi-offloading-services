#ifndef SITE_CLIENTS_H
#define SITE_CLIENTS_H

#include "services/device-context.h"
#include "services/mqtt.h"
#include "services/nds.h"

typedef enum {
    Connect,
    Disconnect
} SiteEventType;

typedef struct {
    SiteEventType type;
    char mac[18];
} SiteEvent;

void init_site_clients(Mosq *mosq, Site *site, NdsClient *nds_client);

#endif // SITE_CLIENTS_H
