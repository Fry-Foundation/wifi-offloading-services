#include "key_pair.h"
#include "lib/console.h"
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <stdbool.h>

#define KEY_PATH_SIZE 512

// Generate a new key pair; note that we alias the type to control the allowed key algorithms
EVP_PKEY *generate_key_pair(GenerateKeyPairType type) {
    // Create the context for key generation
    EVP_PKEY_CTX *pkey_ctx = EVP_PKEY_CTX_new_id(type, NULL);
    if (pkey_ctx == NULL) {
        console(CONSOLE_ERROR, "Error creating PKEY context");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    // Initialize the key generation
    if (EVP_PKEY_keygen_init(pkey_ctx) <= 0) {
        console(CONSOLE_ERROR, "Error initializing PKEY keygen");
        ERR_print_errors_fp(stderr);
        EVP_PKEY_CTX_free(pkey_ctx);
        return NULL;
    }

    // Generate the key pair
    EVP_PKEY *pkey = NULL;
    if (EVP_PKEY_keygen(pkey_ctx, &pkey) <= 0) {
        console(CONSOLE_ERROR, "Error generating key pair");
        ERR_print_errors_fp(stderr);
        EVP_PKEY_CTX_free(pkey_ctx);
        return NULL;
    }

    // Context cleanup
    EVP_PKEY_CTX_free(pkey_ctx);

    return pkey;
}

// Save the private key of the keypair `pkey` to a file in PEM format
bool save_private_key_in_pem(EVP_PKEY *pkey, char *private_key_filepath) {
    FILE *private_key_file = fopen(private_key_filepath, "wb");
    if (!private_key_file) {
        console(CONSOLE_ERROR, "Error opening private key file for writing");
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (PEM_write_PrivateKey(private_key_file, pkey, NULL, NULL, 0, NULL, NULL) != 1) {
        console(CONSOLE_ERROR, "Error writing private key to file");
        ERR_print_errors_fp(stderr);
        fclose(private_key_file);
        return false;
    }

    fclose(private_key_file);
    console(CONSOLE_INFO, "Private key written to %s", private_key_filepath);
    return true;
}

// Save the public key of the keypair `pkey` to a file in PEM format
bool save_public_key_in_pem(EVP_PKEY *pkey, char *public_key_filepath) {
    FILE *public_key_file = fopen(public_key_filepath, "wb");
    if (!public_key_file) {
        console(CONSOLE_ERROR, "Error opening public key file for writing");
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (PEM_write_PUBKEY(public_key_file, pkey) != 1) {
        console(CONSOLE_ERROR, "Error writing public key to file");
        ERR_print_errors_fp(stderr);
        fclose(public_key_file);
        return false;
    }

    fclose(public_key_file);
    console(CONSOLE_INFO, "Public key written to %s", public_key_filepath);
    return true;
}

// Load a private key from a file in PEM format
EVP_PKEY *load_private_key_from_pem(char *private_key_filepath) {
    FILE *private_key_file = fopen(private_key_filepath, "rb");
    if (!private_key_file) {
        console(CONSOLE_ERROR, "Error opening private key file for reading");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    EVP_PKEY *pkey = PEM_read_PrivateKey(private_key_file, NULL, NULL, NULL);
    if (!pkey) {
        console(CONSOLE_ERROR, "Error reading private key from file");
        ERR_print_errors_fp(stderr);
        fclose(private_key_file);
        return NULL;
    }

    fclose(private_key_file);
    return pkey;
}

// Get the public key of the keypair `pkey` as a PEM formatted string using OpenSSL BIO (Basic I/O)
char *get_public_key_pem_string(EVP_PKEY *pkey) {
    BIO *bio = BIO_new(BIO_s_mem());
    if (!bio) {
        console(CONSOLE_ERROR, "Error creating BIO");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    if (PEM_write_bio_PUBKEY(bio, pkey) != 1) {
        console(CONSOLE_ERROR, "Error writing public key to BIO");
        ERR_print_errors_fp(stderr);
        BIO_free(bio);
        return NULL;
    }

    char *public_key_pem_string;
    long bio_size = BIO_get_mem_data(bio, &public_key_pem_string);
    char *public_key_pem_string_copy = malloc(bio_size + 1);
    if (!public_key_pem_string_copy) {
        console(CONSOLE_ERROR, "Error allocating memory for public key string");
        ERR_print_errors_fp(stderr);
        BIO_free(bio);
        return NULL;
    }

    memcpy(public_key_pem_string_copy, public_key_pem_string, bio_size);
    public_key_pem_string_copy[bio_size] = '\0';

    BIO_free(bio);

    return public_key_pem_string_copy;
}
