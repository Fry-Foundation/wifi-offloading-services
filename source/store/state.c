#include "state.h"
#include "../services/access.h"
#include <stdio.h>
#include <stdlib.h>

State state;

void init_state(int mode, AccessKey *access_key) {
    state.mode = mode;

    state.access_key = access_key;

    state.access_status = 0;

    state.setup = 0;

    state.accounting = 0;

    state.on_boot = 1;

    state.already_disabled_wifi = 0;

    // @todo: set this based on what the backend knows about the setup request
    // we are setting it to peaq for DID experiments (for now)
    state.chain = 1;
}

void clean_state() {
    if (state.access_key != NULL) {
        free(state.access_key->key);
        free(state.access_key);
    }
}
