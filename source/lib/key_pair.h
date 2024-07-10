#ifndef KEY_PAIR_H
#define KEY_PAIR_H

// #include <openssl/provider.h>
#include <stdbool.h>
#include <openssl/evp.h>

// OSSL_PROVIDER* init_openssl();
// void cleanup_openssl(OSSL_PROVIDER *provider, EVP_PKEY *pkey);
EVP_PKEY* generate_ed25519_key_pair();
void save_private_key_in_pem(EVP_PKEY *pkey, char *private_key_filepath);
void save_public_key_in_pem(EVP_PKEY *pkey, char *public_key_filepath);
EVP_PKEY* load_private_key_from_pem(char *private_key_filepath);
char *get_public_key_pem_string(EVP_PKEY *pkey);
bool is_valid_base64(const char *str, size_t len);
void remove_whitespace_and_newline_characters(char *str);
char *strip_pem_headers_and_footers(char *public_key_pem_string);

#endif /* KEY_PAIR_H  */
