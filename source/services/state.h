#ifndef STATE_H
#define STATE_H

#include "access.h"

typedef struct {
    // Setup
    int setup;

    // Accounting
    int accounting;

    // On boot
    int on_boot;

    // Chain
    // 0 - algo
    // 1 - peaq
    // 2 - iotex
    int chain;
} State;

extern State state;

void init_state();

#endif // STATE_H
