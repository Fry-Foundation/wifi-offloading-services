#ifndef STATE_H
#define STATE_H

#include <pthread.h>

typedef struct
{
    // Program modes:
    // 0 - onboarding
    // 1 - operator
    int mode;

    // Server state
    // 0 - stopped
    // 1 - running
    int server;
    pthread_cond_t serverCond;
    pthread_mutex_t mutex;
} State;

extern State state;

void initState(int mode);

void cleanState();

#endif // STATE_H
