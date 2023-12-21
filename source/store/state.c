#include <stdlib.h>
#include "state.h"
#include <stdio.h>
#include "../services/access.h"

State state;

void initState(int mode, AccessKey *accessKey)
{
    state.mode = mode;

    state.accessKey = accessKey;

    state.accessStatus = 0;

    state.setup = 0;

    state.accounting = 0;

    state.onBoot = 1;
}

void cleanState()
{
    if (state.accessKey != NULL)
    {
        free(state.accessKey->key);
        free(state.accessKey);
    }
}