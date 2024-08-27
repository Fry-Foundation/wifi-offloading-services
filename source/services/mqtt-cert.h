#ifndef CERT_H
#define CERT_H

#include "services/access_token.h"

void get_ca_cert(AccessToken *access_token);
void generate_and_sign_cert(AccessToken *access_token);

#endif /* CERT_H */
