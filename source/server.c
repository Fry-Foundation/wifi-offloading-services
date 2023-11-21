#include "server.h"
#include "shared_store.h"
#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PORT 48500
#define WAYRU_OS_VERSION "2.0.0"
#define WAYRU_OS_SERVICES_VERSION "1.0.0"

static int answerToConnection(void *cls, struct MHD_Connection *connection,
                              const char *url, const char *method,
                              const char *version, const char *upload_data,
                              size_t *upload_data_size, void **con_cls)
{
    // Example response
    const char *response_text = "Hello from the server!";
    struct MHD_Response *response;
    int ret;

    // For simplicity, we respond to all GET requests the same way
    if (strcmp(method, "GET") == 0)
    {
        response = MHD_create_response_from_buffer(
            strlen(response_text),
            (void *)response_text,
            MHD_RESPMEM_PERSISTENT
        );
        ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    return MHD_NO; // If not GET, respond with error

    // const char *page = "<html><body>Invalid endpoint</body></html>";
    // struct MHD_Response *response;
    // int ret;

    // if (strcmp(method, "GET") != 0 || strcmp(url, "/api/data") != 0)
    //     return MHD_NO; // Only accept GET requests at "/api/data"

    // char json_response[1024];

    // pthread_mutex_lock(&sharedStore.mutex);
    // snprintf(json_response, sizeof(json_response),
    //          "{ \"id\": \"%s\", \"mac\": \"%s\", \"model\": \"%s\", "
    //          "\"wayru_os_version\": \"%s\", \"wayru_os_services_version\": \"%s\" }",
    //          sharedStore.id, sharedStore.mac, sharedStore.model, WAYRU_OS_VERSION, WAYRU_OS_SERVICES_VERSION);
    // pthread_mutex_unlock(&sharedStore.mutex);

    // response = MHD_create_response_from_buffer(strlen(json_response),
    //                                            (void *)json_response,
    //                                            MHD_RESPMEM_MUST_COPY);
    // MHD_add_response_header(response, "Content-Type", "application/json");
    // ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    // MHD_destroy_response(response);

    // return ret;
}

void startHttpServer()
{
    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, PORT,
                                                 NULL, NULL, &answerToConnection,
                                                 NULL, MHD_OPTION_END);
    if (NULL == daemon)
    {
        fprintf(stderr, "Failed to start the HTTP server.\n");
        return;
    }


    // Inform that the server has started
    printf("HTTP server started on port %d\n", PORT);

    pthread_mutex_lock(&sharedStore.mutex);
    while (&sharedStore.runServer) {
        pthread_cond_wait(&sharedStore.serverCond, &sharedStore.mutex);
    }
    pthread_mutex_unlock(&sharedStore.mutex);    

    // Keep the server running until a signal or condition dictates otherwise
    // For instance, you could wait for a condition or signal here
    MHD_stop_daemon(daemon);
}

void stopHttpServer()
{
    pthread_mutex_lock(&sharedStore.mutex);
    sharedStore.runServer = 0;
    pthread_cond_signal(&sharedStore.serverCond);
    pthread_mutex_unlock(&sharedStore.mutex);
}