#include "did-key.h"
#include "lib/console.h"
#include "lib/key_pair.h"
#include "lib/requests.h"
#include "services/access.h"
#include "services/config.h"
#include <json-c/json.h>
#include <stdbool.h>
#include <string.h>
#include <openssl/evp.h>

#define DID_KEY_DIR "did_key"
#define PRIVKEY_FILE_NAME "key"
#define PUBKEY_FILE_NAME "key.pub"
#define KEY_PATH_SIZE 512
#define KEY_GENERATION_RETRIES 5

// Get the public key, and if does not exist, generate a new one
char *get_did_public_key() {
    // Init the OpenSSL provider
    // OSSL_PROVIDER *provider = init_openssl();

    // Load the private key
    char private_key_filepath[KEY_PATH_SIZE];
    snprintf(private_key_filepath, sizeof(private_key_filepath), "%s/%s/%s", config.data_path, DID_KEY_DIR,
             PRIVKEY_FILE_NAME);

    console(CONSOLE_INFO, "Loading private key from %s", private_key_filepath);

    EVP_PKEY *pkey = load_private_key_from_pem(PRIVKEY_FILE_NAME);

    // If the private key does not exist, generate a new key pair
    if (pkey == NULL) {
        pkey = generate_ed25519_key_pair();

        // Save the private key
        save_private_key_in_pem(pkey, private_key_filepath);

        // Save the public key
        char public_key_filepath[KEY_PATH_SIZE];
        snprintf(public_key_filepath, sizeof(public_key_filepath), "%s/%s/%s", config.data_path, DID_KEY_DIR,
                 PUBKEY_FILE_NAME);
        save_public_key_in_pem(pkey, public_key_filepath);
    }

    // Get the public key in PEM format
    char *public_key_pem = get_public_key_pem_string(pkey);

    // Trim the PEM headers and footers
    public_key_pem = strip_pem_headers_and_footers(public_key_pem);

    // Remove whitespace and newlines
    remove_whitespace_and_newline_characters(public_key_pem);

    // Validate the public key
    bool result = is_valid_base64(public_key_pem, strlen(public_key_pem));
    if (!result) {
        console(CONSOLE_ERROR, "Public key validation failed");
        EVP_PKEY_free(pkey);
        exit(1);
    }

    // Cleanup
    // cleanup_openssl(provider, pkey);

    return public_key_pem;
}

char *get_did_key_or_generate() {
    int attempts = 0;
    char *public_key = NULL;
    while (attempts < KEY_GENERATION_RETRIES) {
        public_key = get_did_public_key();
        if (public_key != NULL) {
            return public_key;
        }
        attempts++;
    }

    console(CONSOLE_ERROR, "Failed to generate public key after %d attempts", KEY_GENERATION_RETRIES);
    exit(1);
}