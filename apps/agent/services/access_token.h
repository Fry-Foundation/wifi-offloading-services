#ifndef ACCESS_TOKEN_H
#define ACCESS_TOKEN_H

#include "core/uloop_scheduler.h"
#include "services/callbacks.h"
#include "services/registration.h"
#include <time.h>

typedef struct AccessToken {
    char *token;
    time_t issued_at_seconds;
    time_t expires_at_seconds;
} AccessToken;

typedef struct {
    AccessToken *access_token;
    Registration *registration;
    AccessTokenCallbacks *callbacks;
    task_id_t task_id; // Store current task ID for rescheduling
} AccessTokenTaskContext;

AccessToken *init_access_token(Registration *registration);
AccessTokenTaskContext *
access_token_service(AccessToken *access_token, Registration *registration, AccessTokenCallbacks *callbacks);
void clean_access_token(AccessToken *access_token);
void clean_access_token_context(AccessTokenTaskContext *context);
bool is_token_valid(AccessToken *access_token);

#endif /* ACCESS_TOKEN_H */
