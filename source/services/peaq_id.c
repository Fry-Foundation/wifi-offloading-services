#include "config.h"
#include "../store/state.h"
#include "../utils/console.h"
#include "../utils/requests.h"
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <stdbool.h>
#include <json-c/json.h>

#define KEY_PATH_SIZE 512
#define PRIVKEY_FILE_NAME "peaq_key"
#define PUBKEY_FILE_NAME "peaq_key.pub"
#define PEAQ_ID_ENDPOINT "/api/did/device/peaq"

bool generate_key_pair() {
    // 1. CHeck if a key-pair already exists, and continue if not
    // 2. Summon openssl-util command ot generate key pair at specific location
    // @todo: Encrypt private key
    // @todo: Save private key in location that can be persisted across updates

    char private_key_path[KEY_PATH_SIZE];
    char public_key_path[KEY_PATH_SIZE];

    snprintf(private_key_path, sizeof(private_key_path), "%s/%s", config.data_path, PRIVKEY_FILE_NAME);
    snprintf(public_key_path, sizeof(public_key_path), "%s/%s", config.data_path, PUBKEY_FILE_NAME);

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

char *read_private_key() {
    // 1. Open private key file
    // 2. Read file contents
    console(CONSOLE_DEBUG, "reading peaq private key");

    char privkey_file_path[256];
    snprintf(privkey_file_path, sizeof(privkey_file_path), "%s/%s", config.data_path, PRIVKEY_FILE_NAME);

    FILE *file = fopen(privkey_file_path, "r");
    if (file == NULL) {
        console(CONSOLE_ERROR, "failed to open peaq privkey file");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *file_data_str = malloc(fsize + 1);
    fread(file_data_str, 1, fsize, file);
    fclose(file);

    console(CONSOLE_DEBUG, file_data_str);
    return file_data_str;
}

char *read_public_key() {
    // 1. Open public key file
    // 2. Read file contents

    console(CONSOLE_DEBUG, "reading peaq public key");

    char pubkey_file_path[256];
    snprintf(pubkey_file_path, sizeof(pubkey_file_path), "%s/%s", config.data_path, PUBKEY_FILE_NAME);

    FILE *file = fopen(pubkey_file_path, "r");
    if (file == NULL) {
        console(CONSOLE_ERROR, "failed to open peaq pubkey file");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *file_data_str = malloc(fsize + 1);
    fread(file_data_str, 1, fsize, file);
    fclose(file);

    console(CONSOLE_DEBUG, file_data_str);
    return file_data_str;
}

size_t process_peaq_id_response(char *ptr, size_t size, size_t nmemb, void *userdata) {
    console(CONSOLE_DEBUG, "processing peaq id response");
    console(CONSOLE_DEBUG, "ptr: %s", ptr);

    size_t realsize = size * nmemb;

    // Parse JSON
    struct json_object *parsed_response;
    struct json_object *did_value;

    // Parse the response as JSON
    parsed_response = json_tokener_parse(ptr);
    if (parsed_response == NULL) {
        // JSON parsing failed
        console(CONSOLE_ERROR, "failed to parse accounting response JSON");
        return realsize;
    }

    // Extract the 'did' value from the parsed JSON object
    if (json_object_object_get_ex(parsed_response, "did", &did_value)) {
        if (json_object_is_type(did_value, json_type_string)) {
            const char *did_str = json_object_get_string(did_value);
            console(CONSOLE_DEBUG, "did: %s", did_str);
            // Optionally, handle the 'did' value (store, process further, etc.)
            // ... @todo implement
        } else {
            console(CONSOLE_ERROR, "did key is not a string");
        }
    } else {
        console(CONSOLE_ERROR, "did key not found in JSON response");
    }

    json_object_put(parsed_response);
    return realsize;
}

void post_peaq_id_request(char *public_key) {
    // Build peaq ID URL
    char peaq_id_url[256];
    snprintf(peaq_id_url, sizeof(peaq_id_url), "%s%s", config.main_api,
             PEAQ_ID_ENDPOINT);

    char body[512];
    struct json_object *jobj = json_object_new_object();
    // Create a JSON string containing the public key
    struct json_object *public_key_json = json_object_new_string(public_key);
    // Add the public key to the JSON object
    json_object_object_add(jobj, "peaq_public_key", public_key_json);
    // Print the JSON object as a string
    printf("JSON object created: %s\n", json_object_to_json_string(jobj));

    const char *request_body = json_object_to_json_string(jobj); 

    console(CONSOLE_DEBUG, "peaq_id_url: %s", peaq_id_url);
    console(CONSOLE_DEBUG, "posting peaq id request");
    PostRequestOptions post_peaq_id_options = {
        .url = "http://localhost:3000/peaq",
        .key = state.access_key->key,
        .body = request_body,
        .filePath = NULL,
        .writeFunction = process_peaq_id_response,
        .writeData = NULL,
    };

    performHttpPost(&post_peaq_id_options);

    // Free memory allocated to JSON object
    json_object_put(jobj);
}

void peaq_id_task() {
    // Check if the device is ready to generate a peaq_id
    // Mint complete and the proper chain (peaq) are a must!
    // if(state.access_status == 4 && state.chain == 1) {
    if (1) {
        console(CONSOLE_INFO, "Conditions met to generate peaq ID keys");
        generate_key_pair();
        char *public_key = read_public_key();
        post_peaq_id_request(public_key);
    } else {
        console(CONSOLE_INFO, "Conditions not met for Peaq ID keys");
    }
}
