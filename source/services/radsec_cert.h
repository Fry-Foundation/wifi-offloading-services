#ifndef RADSEC_CERT_H
#define RADSEC_CERT_H

#include "services/access_token.h"
#include "services/registration.h"
#include <stdbool.h>

#define RADSEC_CA_FILE_NAME "radsec-ca.crt"
#define RADSEC_KEY_FILE_NAME "radsec.key"
#define RADSEC_CSR_FILE_NAME "radsec.csr"
#define RADSEC_CERT_FILE_NAME "radsec.crt"

bool attempt_radsec_ca_cert(AccessToken *access_token);
bool attempt_generate_and_sign_radsec(AccessToken *access_token, Registration *registration);
void install_radsec_cert();

#endif /* RADSEC_CERT_H */
