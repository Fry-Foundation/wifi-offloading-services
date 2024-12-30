#include "did-key.h"
#include "lib/console.h"
#include "lib/key_pair.h"
#include "services/config.h"
#include <ctype.h>
#include <openssl/evp.h>
#include <stdbool.h>
#include <string.h>

#define DID_KEY_DIR "did-key"
#define PRIVKEY_FILE_NAME "key"
#define PUBKEY_FILE_NAME "key.pub"
#define KEY_PATH_SIZE 512
#define KEY_GENERATION_RETRIES 5

static Console csl = {
    .topic = "did-key",
};

// Remove whitespace and newline characters from a string
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

// Strip the PEM headers and footers from a public key PEM string
char *strip_pem_headers_and_footers(char *public_key_pem_string) {
    const char *begin = "-----BEGIN PUBLIC KEY-----";
    const char *end = "-----END PUBLIC KEY-----";

    char *begin_pos = strstr(public_key_pem_string, begin);
    if (begin_pos == NULL) {
        print_error(&csl, "Invalid public key PEM format");
        return NULL;
    }

    char *end_pos = strstr(public_key_pem_string, end);
    if (end_pos == NULL) {
        print_error(&csl, "Invalid public key PEM format");
        return NULL;
    }

    begin_pos += strlen(begin);
    size_t stripped_pem_len = end_pos - begin_pos;

    char *stripped_pem = malloc(stripped_pem_len + 1);
    if (!stripped_pem) {
        print_error(&csl, "Error allocating memory for stripped PEM");
        return NULL;
    }

    strncpy(stripped_pem, begin_pos, stripped_pem_len);
    stripped_pem[stripped_pem_len] = '\0';

    return stripped_pem;
}

// Validate a base64 string
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

// Get the public key, and if it does not exist, generate a new one
char *get_did_public_key_or_generate_keypair() {
    // Load the private key
    char private_key_filepath[KEY_PATH_SIZE];
    snprintf(private_key_filepath, sizeof(private_key_filepath), "%s/%s/%s", config.data_path, DID_KEY_DIR,
             PRIVKEY_FILE_NAME);

    print_debug(&csl, "Attempting to load private key from %s", private_key_filepath);
    EVP_PKEY *pkey = load_private_key_from_pem(private_key_filepath);

    if (pkey != NULL) {
        print_debug(&csl, "Private key loaded successfully");
        char *public_key_pem = get_public_key_pem_string(pkey);
        EVP_PKEY_free(pkey);
        public_key_pem = strip_pem_headers_and_footers(public_key_pem);
        remove_whitespace_and_newline_characters(public_key_pem);
        return public_key_pem;
    } else {
        print_debug(&csl, "Private key not found, generating new key pair");
        int attempts = 0;
        while (attempts < KEY_GENERATION_RETRIES) {
            pkey = generate_key_pair(Ed25519);
            if (pkey != NULL) {
                // Extract and format public key content in backend storage format (without headers nor footers)
                char *public_key_pem = get_public_key_pem_string(pkey);
                public_key_pem = strip_pem_headers_and_footers(public_key_pem);
                remove_whitespace_and_newline_characters(public_key_pem);

                // Validate the public key content
                bool result = is_valid_base64(public_key_pem, strlen(public_key_pem));
                if (result) {
                    // Save the private key
                    bool save_private_result = save_private_key_in_pem(pkey, private_key_filepath);
                    if (!save_private_result) {
                        print_error(&csl, "Failed to save private key");
                        exit(1);
                    }

                    // Save the public key
                    char public_key_filepath[KEY_PATH_SIZE];
                    snprintf(public_key_filepath, sizeof(public_key_filepath), "%s/%s/%s", config.data_path,
                             DID_KEY_DIR, PUBKEY_FILE_NAME);

                    bool save_public_result = save_public_key_in_pem(pkey, public_key_filepath);
                    if (!save_public_result) {
                        print_error(&csl, "Failed to save public key");
                        exit(1);
                    }

                    print_info(&csl, "DID key pair generated successfully");

                    EVP_PKEY_free(pkey);
                    return public_key_pem;
                }
            }

            attempts++;
        }

        print_error(&csl, "Failed to generate key pair after %d attempts", KEY_GENERATION_RETRIES);
        exit(1);
    }
}
