#include <stdlib.h>
#include <pthread.h>
#include "state.h"
#include <stdio.h>

State state;

void initState(int mode)
{
    state.mode = mode;
    state.server = 1;
    pthread_cond_init(&state.serverCond, NULL);
    pthread_mutex_init(&state.mutex, NULL);
}

void cleanState()
{
    pthread_mutex_destroy(&state.mutex);
    pthread_cond_destroy(&state.serverCond);
}