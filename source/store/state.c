#include <stdlib.h>
#include "state.h"
#include <stdio.h>
#include "../services/access.h"

State state;

void initState(int mode, AccessKey *access_key)
{
    state.mode = mode;

    state.access_key = access_key;

    state.access_status = 0;

    state.setup = 0;

    state.accounting = 0;

    state.onBoot = 1;

    state.already_disabled_wifi = 0;

    // @todo: set this based on what the backend knows about the setup request
    // we are setting it to peaq for DID experiments (for now)
    state.chain = 1;
}

void cleanState()
{
    if (state.access_key != NULL)
    {
        free(state.access_key->key);
        free(state.access_key);
    }
}
