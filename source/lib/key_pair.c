#include "key_pair.h"
#include "../services/config.h"
#include "console.h"
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <stdbool.h>

#define KEY_PATH_SIZE 512

bool generate_key_pair(char *public_key_filename, char *private_key_filename) {
    // 1. Check if a key-pair already exists, and continue if not
    // 2. Summon openssl-util command ot generate key pair at specific location
    // @todo: Encrypt private key
    // @todo: Save private key in location that can be persisted across updates

    char private_key_path[KEY_PATH_SIZE];
    char public_key_path[KEY_PATH_SIZE];

    snprintf(private_key_path, sizeof(private_key_path), "%s/%s", config.data_path, private_key_filename);

    snprintf(public_key_path, sizeof(public_key_path), "%s/%s", config.data_path, public_key_filename);

    console(CONSOLE_INFO, "priv key filename %s", private_key_path);
    console(CONSOLE_INFO, "pub key filename %s", public_key_path);

    bool success = false;
    RSA *r = NULL;
    BIGNUM *bne = NULL;
    BIO *bp_public = NULL, *bp_private = NULL;

    int bits = 2048;
    unsigned long e = RSA_F4;

    // Create the BIGNUM object for the exponent
    bne = BN_new();
    if (!bne) {
        goto free_all;
    }

    if (BN_set_word(bne, e) != 1) {
        goto free_all;
    }

    // Generate RSA key pair
    r = RSA_new();
    if (!r) {
        goto free_all;
    }

    if (RSA_generate_key_ex(r, bits, bne, NULL) != 1) {
        goto free_all;
    }

    // Save public key
    bp_public = BIO_new_file(public_key_path, "w+");
    if (!bp_public) {
        goto free_all;
    }

    if (PEM_write_bio_RSAPublicKey(bp_public, r) != 1) {
        goto free_all;
    }

    // Save private key
    bp_private = BIO_new_file(private_key_path, "w+");
    if (!bp_private) {
        goto free_all;
    }

    if (PEM_write_bio_RSAPrivateKey(bp_private, r, NULL, NULL, 0, NULL, NULL) != 1) {
        goto free_all;
    }

    success = true; // Set success to true if all operations succeeded

free_all:
    if (bp_public)
        BIO_free_all(bp_public);
    if (bp_private)
        BIO_free_all(bp_private);
    if (r)
        RSA_free(r);
    if (bne)
        BN_free(bne);

    return success;
}