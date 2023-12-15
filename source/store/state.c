#include <stdlib.h>
#include <pthread.h>
#include "state.h"
#include <stdio.h>
#include "../services/access.h"

State state;

void initState(int mode, AccessKey *accessKey)
{
    state.mode = mode;
    state.server = 1;
    pthread_cond_init(&state.serverCond, NULL);
    pthread_mutex_init(&state.serverMutex, NULL);

    state.accessKey = accessKey;

    state.accessStatus = 0;

    state.setup = 0;

    state.accounting = 0;
}

void cleanState()
{
    if (state.accessKey != NULL)
    {
        free(state.accessKey->key);
        free(state.accessKey);
    }

    pthread_cond_destroy(&state.serverCond);
    pthread_mutex_destroy(&state.serverMutex);
}