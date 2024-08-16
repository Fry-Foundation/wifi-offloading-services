#ifndef SITE_CLIENTS_H
#define SITE_CLIENTS_H

#include "lib/scheduler.h"
#include "mosquitto.h"
#include "services/device-context.h"

int init_site_clients_fifo();
void site_clients_service(Scheduler *sch, struct mosquitto *mosq, int site_fifo_fd, Site *site);
void clean_site_clients_fifo(int site_fifo_fd);

#endif // SITE_CLIENTS_H
