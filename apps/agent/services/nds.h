#ifndef NDS_H
#define NDS_H

// NDS stands for Network Demarcation Service
// This is our OpenNDS integration

#include "core/uloop_scheduler.h"
#include "services/device-context.h"
#include "services/device_info.h"
#include "services/mqtt/mqtt.h"

#define MAC_ADDR_LEN 18 // Standard MAC address length (17 chars + null terminator)

typedef struct {
    bool opennds_installed;
    int fifo_fd;
} NdsClient;

typedef struct {
    NdsClient *client;
    Mosq *mosq;
    Site *site;
    DeviceInfo *device_info;
    task_id_t task_id;  // Store current task ID for cleanup
} NdsTaskContext;

NdsClient *init_nds_client();
NdsTaskContext *nds_service(Mosq *mosq, Site *site, NdsClient *nds_client, DeviceInfo *device_info);
void clean_nds_context(NdsTaskContext *context);
void clean_nds_fifo(int *nds_fifo_fd);

#endif /* NDS_H */
