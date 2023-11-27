#include <stdio.h>
#include "accounting.h"
#include "../store/state.h"

void queryOpenNds()
{
    printf("[accounting] querying OpenNDS\n");
}

void postAccountingUpdate()
{
    printf("[accounting] posting accounting update\n");
}

void deauthenticateSessions()
{
    printf("[accounting] ending sessions\n");
}

void accountingTask()
{
    if (state.accounting != 1) {
        printf("[accounting] accounting is disabled\n");
        return;
    }

    printf("[accounting] ccounting task\n");

    queryOpenNds();
    postAccountingUpdate();
    deauthenticateSessions();
}