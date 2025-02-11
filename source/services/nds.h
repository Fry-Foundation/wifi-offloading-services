#ifndef NDS_H
#define NDS_H

// NDS stands for Network Demarcation Service
// This is our OpenNDS integration

#include "lib/scheduler.h"
#include "services/device-context.h"
#include "services/mqtt.h"

#define MAC_ADDR_LEN 18  // Standard MAC address length (17 chars + null terminator)

typedef struct {
    bool opennds_installed;
    int fifo_fd;
} NdsClient;

typedef struct {
    NdsClient *client;
    Mosq *mosq;
    Site *site;
} NdsTaskContext;

NdsClient *init_nds_client();
void nds_service(Scheduler *sch, Mosq *mosq, Site *site, NdsClient *nds_client);
void clean_nds_fifo(int *nds_fifo_fd);

#endif /* NDS_H */
