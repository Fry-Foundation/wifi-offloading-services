#ifndef STATE_H
#define STATE_H

#include <pthread.h>
#include "../services/access.h"

typedef struct
{
    // Program modes:
    // 0 - onboarding
    // 1 - operator
    int mode;

    // Access key
    AccessKey* accessKey;

    // Access status
    // 0 - initial
    // 1 - banned
    // 2 - setup-pending
    // 3 - setup-approved
    // 4 - setup-completed
    int accessStatus;

    // Setup
    int setup;

    // Accounting
    int accounting;

    // On boot
    int onBoot;
} State;

extern State state;

void initState(int mode, AccessKey *accessKey);

void cleanState();

#endif // STATE_H
