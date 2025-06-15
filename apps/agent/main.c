#include "core/console.h"
#include "core/result.h"
#include "core/scheduler.h"
// #include "services/access_token.h"

static Console csl = {
    .topic = "main",
};

int main(int argc, char *argv[]) {
    console_info(&csl, "starting wayru-os-services");

    Result res;
    res.ok = true;
    console_info(&csl, "Result initialized successfully %d", res.ok);

    Scheduler *sch = init_scheduler();
    run_tasks(sch);

    // Access token
    // AccessToken *access_token = init_access_token(registration);
    // if (access_token == NULL) {
    //     console_error(&csl, "Failed to start access token ... exiting");
    //     // cleanup_and_exit(1, "Failed to initialize access token");
    // }

    return 0;
}
