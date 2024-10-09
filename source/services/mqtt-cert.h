#ifndef CERT_H
#define CERT_H

#include "services/access_token.h"
#include <stdbool.h>


bool attempt_ca_cert(AccessToken *access_token);
bool attempt_generate_and_sign(AccessToken *access_token);

#endif /* CERT_H */
