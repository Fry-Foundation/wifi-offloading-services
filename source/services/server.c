#include "microhttpd.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <json-c/json.h>
#include "server.h"
#include "../store/config.h"
#include "../store/state.h"

#define PORT 40500

static enum MHD_Result answerToConnection(
    void *cls,
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls)
{
    struct MHD_Response *response;
    enum MHD_Result ret;

    // We respond to GET requests at "/api/device-data"
    if (strcmp(method, "GET") != 0 || strcmp(url, "/api/device-data") != 0)
    {
        return MHD_NO;
    }

    // Build response with device data from shared store
    json_object *jsonData = json_object_new_object();
    char json_response[1024];
    json_object_object_add(jsonData, "id", json_object_new_string(getConfig().deviceId));
    const char *jsonDataString = json_object_to_json_string(jsonData);

    response = MHD_create_response_from_buffer(
        strlen(jsonDataString),
        (void *)jsonDataString,
        MHD_RESPMEM_MUST_COPY);

    MHD_add_response_header(response, "Content-Type", "application/json");
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    json_object_put(jsonData);

    return ret;
}

void startHttpServer()
{
    struct MHD_Daemon *daemon = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD,
        PORT,
        NULL,
        NULL,
        &answerToConnection,
        NULL,
        MHD_OPTION_END);

    if (NULL == daemon)
    {
        fprintf(stderr, "[server] failed to start\n");
        return;
    }

    printf("[server] running on port %d\n", PORT);

    pthread_mutex_lock(&state.serverMutex);
    while (state.server == 1)
    {
        pthread_cond_wait(&state.serverCond, &state.serverMutex);
    }
    pthread_mutex_unlock(&state.serverMutex);

    printf("[server] stopping\n");

    MHD_stop_daemon(daemon);
}

void stopHttpServer()
{
    printf("[server] stop request\n");
    pthread_mutex_lock(&state.serverMutex);
    state.server = 0;
    pthread_cond_signal(&state.serverCond);
    pthread_mutex_unlock(&state.serverMutex);
}