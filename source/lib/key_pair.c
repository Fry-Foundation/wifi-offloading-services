#include "key_pair.h"
#include "lib/console.h"
#include "services/config.h"
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <stdbool.h>
#include <ctype.h>

#define KEY_PATH_SIZE 512

// Generate a new ed25519 key pair
EVP_PKEY *generate_ed25519_key_pair() {
    // Create the context for ed25519 key generation
    EVP_PKEY_CTX *pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
    if (!pkey_ctx) {
        console(CONSOLE_ERROR, "Error creating PKEY context");
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Initialize the key generation
    if (EVP_PKEY_keygen_init(pkey_ctx) <= 0) {
        console(CONSOLE_ERROR, "Error initializing PKEY keygen");
        ERR_print_errors_fp(stderr);
        EVP_PKEY_CTX_free(pkey_ctx);
        return false;
    }

    // Generate the key pair
    EVP_PKEY *pkey = NULL;
    if (EVP_PKEY_keygen(pkey_ctx, &pkey) <= 0) {
        console(CONSOLE_ERROR, "Error generating key pair");
        ERR_print_errors_fp(stderr);
        EVP_PKEY_CTX_free(pkey_ctx);
        return false;
    }

    // Context cleanup
    EVP_PKEY_CTX_free(pkey_ctx);

    return pkey;
}

// Save the private key of the keypair `pkey` to a file in PEM format
void save_private_key_in_pem(EVP_PKEY *pkey, char *private_key_filepath) {
    FILE *private_key_file = fopen(private_key_filepath, "wb");
    if (!private_key_file) {
        console(CONSOLE_ERROR, "Error opening private key file for writing");
        ERR_print_errors_fp(stderr);
        return;
    }

    if (PEM_write_PrivateKey(private_key_file, pkey, NULL, NULL, 0, NULL, NULL) != 1) {
        console(CONSOLE_ERROR, "Error writing private key to file");
        ERR_print_errors_fp(stderr);
        fclose(private_key_file);
        return;
    }

    fclose(private_key_file);
    console(CONSOLE_INFO, "Private key written to %s", private_key_filepath);
}

// Save the public key of the keypair `pkey` to a file in PEM format
void save_public_key_in_pem(EVP_PKEY *pkey, char *public_key_filepath) {
    FILE *public_key_file = fopen(public_key_filepath, "wb");
    if (!public_key_file) {
        console(CONSOLE_ERROR, "Error opening public key file for writing");
        ERR_print_errors_fp(stderr);
        return;
    }

    if (PEM_write_PUBKEY(public_key_file, pkey) != 1) {
        console(CONSOLE_ERROR, "Error writing public key to file");
        ERR_print_errors_fp(stderr);
        fclose(public_key_file);
        return;
    }

    fclose(public_key_file);
    console(CONSOLE_INFO, "Public key written to %s", public_key_filepath);
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

// Check that the PEM public key is a valid base64 encoded string
bool validate_base64_public_key_pem_string(char *public_key_pem_string) {
    BIO *bio = BIO_new(BIO_s_mem());
    if (!bio) {
        console(CONSOLE_ERROR, "Error creating BIO");
        ERR_print_errors_fp(stderr);
        return false;
    }

    BIO_puts(bio, public_key_pem_string);
    EVP_PKEY *pkey = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
    if (!pkey) {
        console(CONSOLE_ERROR, "Error reading public key from BIO");
        ERR_print_errors_fp(stderr);
        BIO_free(bio);
        return false;
    }

    BIO_free(bio);
    EVP_PKEY_free(pkey);
    return true;
}

void remove_whitespace_and_newline_characters(char *str) {
    char *src = str;
    char *dst = str;
    while (*src) {
        if (*src != ' ' && *src != '\n' && *src != '\r') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

char *strip_pem_headers_and_footers(char *public_key_pem_string) {
    const char *begin = "-----BEGIN PUBLIC KEY-----";
    const char *end = "-----END PUBLIC KEY-----";

    char *begin_pos = strstr(public_key_pem_string, begin);
    if (begin_pos == NULL) {
        console(CONSOLE_ERROR, "Invalid public key PEM format");
        return NULL;
    }

    char *end_pos = strstr(public_key_pem_string, end);
    if (end_pos == NULL) {
        console(CONSOLE_ERROR, "Invalid public key PEM format");
        return NULL;
    }

    begin_pos += strlen(begin);
    size_t stripped_pem_len = end_pos - begin_pos;

    char *stripped_pem = malloc(stripped_pem_len + 1);
    if (!stripped_pem) {
        console(CONSOLE_ERROR, "Error allocating memory for stripped PEM");
        return NULL;
    }

    strncpy(stripped_pem, begin_pos, stripped_pem_len);
    stripped_pem[stripped_pem_len] = '\0';

    printf("Stripped PEM: %s\n", stripped_pem);

    // Remove whitespace and newline characters
    remove_whitespace_and_newline_characters(stripped_pem);

    printf("Stripped PEM without whitespace: %s\n", stripped_pem);

    return stripped_pem;
}


bool is_valid_base64(const char *str, size_t length) {
    if (length % 4 != 0) {
        return false;
    }

    for (size_t i = 0; i < length; i++) {
        if (!isalnum(str[i]) && str[i] != '+' && str[i] != '/' && str[i] != '=') {
            return false;
        }
    }

    // Check for correct padding
    if (length > 0 && str[length - 1] == '=') {
        if (length > 1 && str[length - 2] == '=') {
            // Last two characters can be '='
            return true;
        }
        // Only the last character can be '='
        return str[length - 2] != '=';
    }

    return true;
}