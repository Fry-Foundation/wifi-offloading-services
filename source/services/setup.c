#include <stdio.h>
#include "setup.h"
#include "../store/state.h"

void requestSetup()
{
    printf("[setup] request approved setup\n");
}

void checkApprovedSetup()
{
    printf("[setup] check setup\n");
}

void completeSetup()
{
    printf("[setup] complete setup\n");
}

void reboot()
{
    printf("[setup] reboot\n");
}

void setupTask()
{
    if (state.setup != 1) {
        printf("[setup] setup is disabled\n");
        return;
    }

    printf("[setup] setup task\n");
    requestSetup();
    checkApprovedSetup();
    completeSetup();
    reboot();
}