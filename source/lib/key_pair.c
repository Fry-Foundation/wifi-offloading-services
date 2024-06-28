#include "key_pair.h"
#include "lib/console.h"
#include "services/config.h"
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <stdbool.h>

#define KEY_PATH_SIZE 512

bool generate_key_pair(char *public_key_filename, char *private_key_filename) {
    // Initialize OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();

    char private_key_path[KEY_PATH_SIZE];
    char public_key_path[KEY_PATH_SIZE];

    snprintf(private_key_path, sizeof(private_key_path), "%s/%s", config.data_path, private_key_filename);

    snprintf(public_key_path, sizeof(public_key_path), "%s/%s", config.data_path, public_key_filename);

    console(CONSOLE_INFO, "priv key filename %s", private_key_path);
    console(CONSOLE_INFO, "pub key filename %s", public_key_path);

    // Create eypair context
    EVP_PKEY_CTX *pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
    if (!pkey_ctx) {
        fprintf(stderr, "Error creating context\n");
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Generate the keypair
    if (EVP_PKEY_keygen_init(pkey_ctx) <= 0) {
        fprintf(stderr, "Error initializing keygen\n");
        ERR_print_errors_fp(stderr);
        EVP_PKEY_CTX_free(pkey_ctx);
        return false;
    }

    EVP_PKEY *pkey = NULL;
    if (EVP_PKEY_keygen(pkey_ctx, &pkey) <= 0) {
        fprintf(stderr, "Error generating key\n");
        ERR_print_errors_fp(stderr);
        EVP_PKEY_CTX_free(pkey_ctx);
        return false;
    }

    // Write the private key to a file
    FILE *private_key_file = fopen(private_key_path, "wb");
    if (!private_key_file) {
        fprintf(stderr, "Error opening private key file for writing\n");
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(pkey_ctx);
        return false;
    }

    if (PEM_write_PrivateKey(private_key_file, pkey, NULL, NULL, 0, NULL, NULL) != 1) {
        fprintf(stderr, "Error writing private key to file\n");
        ERR_print_errors_fp(stderr);
        fclose(private_key_file);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(pkey_ctx);
        return false;
    }
    fclose(private_key_file);

    // Write the public key to a file
    FILE *public_key_file = fopen(public_key_path, "wb");
    if (!public_key_file) {
        fprintf(stderr, "Error opening public key file for writing\n");
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(pkey_ctx);
        return false;
    }

    if (PEM_write_PUBKEY(public_key_file, pkey) != 1) {
        fprintf(stderr, "Error writing public key to file\n");
        ERR_print_errors_fp(stderr);
        fclose(public_key_file);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(pkey_ctx);
        return false;
    }

    fclose(public_key_file);

    // Cleanup
    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(pkey_ctx);
    EVP_cleanup();
    ERR_free_strings();

    console(CONSOLE_INFO, "Keys generated and written to %s and %s", public_key_filename, private_key_filename);
    return true;
}
