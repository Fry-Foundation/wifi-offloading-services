#ifndef SHARED_STORE_H
#define SHARED_STORE_H

#include <pthread.h>


// mode puede ser 0, 1
// 0: modo onboarding
// 1: modo operador

typedef struct {
    int mode;
    pthread_cond_t serverCond;
    pthread_mutex_t mutex;
} SharedStore;

extern SharedStore sharedStore;

#endif // SHARED_STORE_H
