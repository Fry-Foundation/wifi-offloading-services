#ifndef SHARED_STORE_H
#define SHARED_STORE_H

#include <pthread.h>

typedef struct {
    int devMode;
    char scriptsPath[256];
    char dataPath[256];
    char* id;
    char* mac;
    char* model;
    int runServer;
    pthread_cond_t serverCond;
    pthread_mutex_t mutex;
} SharedStore;

extern SharedStore sharedStore;

#endif // SHARED_STORE_H
