#ifndef ACCESS_TOKEN_H
#define ACCESS_TOKEN_H

#include "lib/scheduler.h"
#include "services/registration.h"
#include <time.h>

typedef struct {
    char *token;
    time_t issued_at_seconds;
    time_t expires_at_seconds;
} AccessToken;

AccessToken *init_access_token(Registration *registration);
void access_token_service(Scheduler *sch, AccessToken *_access_token, Registration *_registration);
void clean_access_token(AccessToken *access_token);

#endif /* ACCESS_TOKEN_H */
