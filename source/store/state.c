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
}

void cleanState()
{
    if (state.access_key != NULL)
    {
        free(state.access_key->key);
        free(state.access_key);
    }
}