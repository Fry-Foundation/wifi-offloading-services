#include "state.h"
#include "access.h"
#include <stdio.h>
#include <stdlib.h>

State state;

void init_state(int mode) {
    state.mode = mode;

    state.access_status = 0;

    state.setup = 0;

    state.accounting = 0;

    state.on_boot = 1;

    state.already_disabled_wifi = 0;

    // @todo: set this based on what the backend knows about the setup request
    // we are setting it to peaq for DID experiments (for now)
    state.chain = 1;
}
