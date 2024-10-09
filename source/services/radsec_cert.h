#ifndef RADSEC_CERT_H
#define RADSEC_CERT_H

#include "services/access_token.h"
#include "services/registration.h"
#include <stdbool.h>

bool attempt_radsec_ca_cert(AccessToken *access_token);
bool attempt_generate_and_sign_radsec(AccessToken *access_token, Registration *registration);

#endif /* RADSEC_CERT_H */
