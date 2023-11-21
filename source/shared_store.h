#ifndef SHARED_STORE_H
#define SHARED_STORE_H

#include <pthread.h>

typedef struct {
    int devMode;
    char scriptsPath[256];
    char dataPath[256];
    char id[256];
    char mac[256];
    char model[256];
    pthread_mutex_t mutex;
} SharedStore;

extern SharedStore sharedStore;

#endif // SHARED_STORE_H
