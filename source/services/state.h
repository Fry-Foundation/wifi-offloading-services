#ifndef STATE_H
#define STATE_H

#include "access.h"

typedef struct {
    // Program modes:
    // 0 - onboarding
    // 1 - operator
    int mode;

    // Access status
    // 0 - initial
    // 1 - setup-pending
    // 2 - setup-approved
    // 3 - mint-pending
    // 4 - ready
    // 5 - banned
    int access_status;

    // Setup
    int setup;

    // Accounting
    int accounting;

    // On boot
    int on_boot;

    // Wireless network
    int already_disabled_wifi;

    // Chain
    // 0 - algo
    // 1 - peaq
    // 2 - iotex
    int chain;
} State;

extern State state;

void init_state(int mode);

void clean_state();

#endif // STATE_H
