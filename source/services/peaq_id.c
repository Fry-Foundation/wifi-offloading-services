#include "config.h"
#include "../store/state.h"
#include "../utils/console.h"
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <stdbool.h>

const int KEY_PATH_BUFFER_SIZE = 1024;

bool generate_key_pair(const char *active_path) {
    // 1. CHeck if a key-pair already exists, and continue if not
    // 2. Summon openssl-util command ot generate key pair at specific location
    // @todo: Encrypt private key
    // @todo: Save private key in location that can be persisted across updates

    char private_key_filename[KEY_PATH_BUFFER_SIZE];
    char public_key_filename[KEY_PATH_BUFFER_SIZE];

    snprintf(private_key_filename, sizeof(private_key_filename), "%s%s", active_path, "/peaq_key");
    snprintf(public_key_filename, sizeof(public_key_filename), "%s%s", active_path,
             "/peaq_key.pub");

    console(CONSOLE_INFO, "priv key filename %s", private_key_filename);
    console(CONSOLE_INFO, "pub key filename %s", public_key_filename);

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
    bp_public = BIO_new_file(public_key_filename, "w+");
    if (!bp_public) {
        goto free_all;
    }

    if (PEM_write_bio_RSAPublicKey(bp_public, r) != 1) {
        goto free_all;
    }

    // Save private key
    bp_private = BIO_new_file(private_key_filename, "w+");
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

char *read_private_key() {
    // @todo Implement, but first fix global active_path
    // 1. Open private key file
    // 2. Read file contents
}

char *read_public_key() {
    // @todo Implement, but first fix global active_path
    // 1. Open public key file
    // 2. Read file contents
}

void peaq_id_task() {
    // Check if the device is ready to generate a peaq_id
    // Mint complete and the proper chain (peaq) are a must!
    // if(state.access_status == 4 && state.chain == 1) {
    if (1) {
        console(CONSOLE_INFO, "Conditions met to generate peaq ID keys");

        const char *dev_path = "./data";
        const char *prod_path = "/etc/wayru-os-services/data";
        const char *active_path;
        if (config.dev_env) {
            active_path = dev_path;
        } else {
            active_path = prod_path;
        }

        generate_key_pair(active_path);
    } else {
        console(CONSOLE_INFO, "Conditions not met for Peaq ID keys");
    }
}
