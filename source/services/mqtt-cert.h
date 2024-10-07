#ifndef CERT_H
#define CERT_H

#include "services/access_token.h"

int get_ca_cert(AccessToken *access_token);
void generate_and_sign_cert(AccessToken *access_token);

#endif /* CERT_H */
