#ifndef KEY_PAIR_H
#define KEY_PAIR_H

#include <openssl/evp.h>
#include <stdbool.h>

typedef enum {
    Rsa = EVP_PKEY_RSA,
    Ed25519 = EVP_PKEY_ED25519,
} GenerateKeyPairType;

EVP_PKEY *generate_key_pair(GenerateKeyPairType type);
bool save_private_key_in_pem(EVP_PKEY *pkey, char *private_key_filepath);
bool save_public_key_in_pem(EVP_PKEY *pkey, char *public_key_filepath);
EVP_PKEY *load_private_key_from_pem(char *private_key_filepath);
char *get_public_key_pem_string(EVP_PKEY *pkey);
void generate_csr(EVP_PKEY *pkey, const char *csr_filepath);
X509 *load_certificate(const char *cert_path);
int verify_certificate(const char *cert_path, const char *ca_cert_path);

#endif /* KEY_PAIR_H  */
