#include "key_pair.h"
#include "lib/console.h"
#include <json-c/json.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
#include <stdbool.h>

#define KEY_PATH_SIZE 512

static Console csl = {
    .topic = "key_pair",
};

// Generate a new key pair; note that we alias the type to control the allowed key algorithms
EVP_PKEY *generate_key_pair(GenerateKeyPairType type) {
    // Create the context for key generation
    EVP_PKEY_CTX *pkey_ctx = EVP_PKEY_CTX_new_id(type, NULL);
    if (pkey_ctx == NULL) {
        console_error(&csl, "Error creating PKEY context");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    // Initialize the key generation
    if (EVP_PKEY_keygen_init(pkey_ctx) <= 0) {
        console_error(&csl, "Error initializing PKEY keygen");
        ERR_print_errors_fp(stderr);
        EVP_PKEY_CTX_free(pkey_ctx);
        return NULL;
    }

    // Generate the key pair
    EVP_PKEY *pkey = NULL;
    if (EVP_PKEY_keygen(pkey_ctx, &pkey) <= 0) {
        console_error(&csl, "Error generating key pair");
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
        console_error(&csl, "Error opening private key file for writing");
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (PEM_write_PrivateKey(private_key_file, pkey, NULL, NULL, 0, NULL, NULL) != 1) {
        console_error(&csl, "Error writing private key to file");
        ERR_print_errors_fp(stderr);
        fclose(private_key_file);
        return false;
    }

    fclose(private_key_file);
    console_debug(&csl, "Private key written to %s", private_key_filepath);
    return true;
}

// Save the public key of the keypair `pkey` to a file in PEM format
bool save_public_key_in_pem(EVP_PKEY *pkey, char *public_key_filepath) {
    FILE *public_key_file = fopen(public_key_filepath, "wb");
    if (!public_key_file) {
        console_error(&csl, "Error opening public key file for writing");
        ERR_print_errors_fp(stderr);
        return false;
    }

    if (PEM_write_PUBKEY(public_key_file, pkey) != 1) {
        console_error(&csl, "Error writing public key to file");
        ERR_print_errors_fp(stderr);
        fclose(public_key_file);
        return false;
    }

    fclose(public_key_file);
    console_debug(&csl, "Public key written to %s", public_key_filepath);
    return true;
}

// Load a private key from a file in PEM format
EVP_PKEY *load_private_key_from_pem(char *private_key_filepath) {
    FILE *private_key_file = fopen(private_key_filepath, "rb");
    if (!private_key_file) {
        console_debug(&csl, "Failed to open private key file for reading");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    EVP_PKEY *pkey = PEM_read_PrivateKey(private_key_file, NULL, NULL, NULL);
    if (!pkey) {
        console_error(&csl, "Error reading private key from file");
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
        console_error(&csl, "Failed to create BIO");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    if (PEM_write_bio_PUBKEY(bio, pkey) != 1) {
        console_error(&csl, "Failed to write public key to BIO");
        ERR_print_errors_fp(stderr);
        BIO_free(bio);
        return NULL;
    }

    char *public_key_pem_string;
    long bio_size = BIO_get_mem_data(bio, &public_key_pem_string);
    char *public_key_pem_string_copy = malloc(bio_size + 1);
    if (!public_key_pem_string_copy) {
        console_error(&csl, "Failed to allocate memory for public key string");
        ERR_print_errors_fp(stderr);
        BIO_free(bio);
        return NULL;
    }

    memcpy(public_key_pem_string_copy, public_key_pem_string, bio_size);
    public_key_pem_string_copy[bio_size] = '\0';

    BIO_free(bio);

    return public_key_pem_string_copy;
}

// Load a certificate from a PEM file
X509 *load_certificate(const char *cert_path) {
    FILE *cert_file = fopen(cert_path, "rb");
    if (!cert_file) {
        console_debug(&csl, "Unable to open certificate file: %s", cert_path);
        return NULL;
    }
    X509 *cert = PEM_read_X509(cert_file, NULL, NULL, NULL);
    fclose(cert_file);
    return cert;
}

// Verify a certificate against a CA certificate
int verify_certificate(const char *cert_path, const char *ca_cert_path) {
    X509 *cert = load_certificate(cert_path);
    if (!cert) {
        console_debug(&csl, "Failed to load certificate: %s", cert_path);
        return 0;
    }

    X509 *ca_cert = load_certificate(ca_cert_path);
    if (!ca_cert) {
        console_debug(&csl, "Failed to load CA certificate: %s", ca_cert_path);
        X509_free(cert);
        return 0;
    }

    X509_STORE *store = X509_STORE_new();
    if (!store) {
        console_error(&csl, "Failed to create X509_STORE");
        X509_free(cert);
        X509_free(ca_cert);
        return 0;
    }
    X509_STORE_add_cert(store, ca_cert);

    X509_STORE_CTX *ctx = X509_STORE_CTX_new();
    if (!ctx) {
        console_error(&csl, "Failed to create X509_STORE_CTX");
        X509_STORE_free(store);
        X509_free(cert);
        X509_free(ca_cert);
        return 0;
    }

    X509_STORE_CTX_init(ctx, store, cert, NULL);

    int ret = X509_verify_cert(ctx);
    if (ret == 1) {
        console_debug(&csl, "Certificate is valid.");
    } else {
        int err = X509_STORE_CTX_get_error(ctx);
        console_error(&csl, "Certificate verification failed: %s", X509_verify_cert_error_string(err));
    }

    X509_STORE_CTX_free(ctx);
    X509_STORE_free(store);
    X509_free(cert);
    X509_free(ca_cert);

    return ret == 1;
}
