#include "peaq_did.h"
#include "lib/console.h"
#include "lib/key_pair.h"
#include "lib/requests.h"
#include "services/access.h"
#include "services/config.h"
#include "services/state.h"
#include <json-c/json.h>
#include <stdbool.h>

#define PRIVKEY_FILE_NAME "peaq_key"
#define PUBKEY_FILE_NAME "peaq_key.pub"
#define PEAQ_DID_CREATE_ENDPOINT "/api/did/peaq/create"
#define PEAQ_DID_READ_ENDPOINT "/api/did/peaq/read"

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

size_t process_peaq_did_create_response(char *ptr, size_t size, size_t nmemb, void *userdata) {
    console(CONSOLE_DEBUG, "processing peaq did create response");
    console(CONSOLE_DEBUG, "ptr: %s", ptr);

    size_t realsize = size * nmemb;

    // Parse JSON
    struct json_object *parsed_response;
    struct json_object *message_value;

    // Parse the response as JSON
    parsed_response = json_tokener_parse(ptr);
    if (parsed_response == NULL) {
        // JSON parsing failed
        console(CONSOLE_ERROR, "failed to parse peaq did create response");
        return realsize;
    }

    // Extract the 'message' value from the parsed JSON object
    if (json_object_object_get_ex(parsed_response, "message", &message_value)) {
        if (json_object_is_type(message_value, json_type_string)) {
            const char *message_str = json_object_get_string(message_value);
            console(CONSOLE_DEBUG, "message: %s", message_str);

            // Handle the 'message' value
            // If success, consider saving a flag that the DID has already been created in a file
            // If failure, consider scheduling a second attempt
        } else {
            console(CONSOLE_ERROR, "message key is not a string");
        }
    } else {
        console(CONSOLE_ERROR, "message key not found in JSON response");
    }

    json_object_put(parsed_response);
    return realsize;
}

size_t process_peaq_did_read_response(char *ptr, size_t size, size_t nmemb, void *userdata) {
    console(CONSOLE_DEBUG, "processing peaq did read response");
    console(CONSOLE_DEBUG, "ptr: %s", ptr);

    size_t realsize = size * nmemb;

    // Parse JSON
    struct json_object *parsed_response;
    struct json_object *message_value;
    struct json_object *did_value;

    // Parse the response as JSON
    parsed_response = json_tokener_parse(ptr);
    if (parsed_response == NULL) {
        // JSON parsing failed
        console(CONSOLE_ERROR, "failed to parse peaq did read response");
        return realsize;
    }

    // Extract the 'message' and 'did' values
    json_object_object_get_ex(parsed_response, "message", &message_value);
    json_object_object_get_ex(parsed_response, "did", &did_value);

    const char *message = json_object_get_string(message_value);
    const char *did = json_object_get_string(did_value);

    if (message == NULL || did == NULL) {
        console(CONSOLE_ERROR, "failed to extract message or did from peaq did read response");
        json_object_put(parsed_response); // Release the parsed JSON object
        return realsize;
    }

    // Do something with the extracted values (e.g., print them)
    console(CONSOLE_INFO, "Received message: %s", message);
    console(CONSOLE_INFO, "Received peaq DID: %s", did);

    json_object_put(parsed_response);
    return realsize;
}

void post_peaq_did_create_request(char *public_key) {
    // Build peaq ID URL
    char create_request_url[256];
    snprintf(create_request_url, sizeof(create_request_url), "%s%s", config.main_api, PEAQ_DID_CREATE_ENDPOINT);

    // Build body
    char body[512];
    struct json_object *jobj = json_object_new_object();
    struct json_object *public_key_json = json_object_new_string(public_key);
    json_object_object_add(jobj, "peaq_public_key", public_key_json);
    printf("JSON object created: %s\n", json_object_to_json_string(jobj));
    const char *request_body = json_object_to_json_string(jobj);
    console(CONSOLE_DEBUG, "JSON body created: %s", request_body);

    // Build request options
    PostRequestOptions post_peaq_did_options = {
        .url = create_request_url,
        .key = access_key.public_key,
        .body = request_body,
        .filePath = NULL,
        .writeFunction = process_peaq_did_create_response,
        .writeData = NULL,
    };

    // Post
    performHttpPost(&post_peaq_did_options);

    // Free memory allocated to JSON object
    json_object_put(jobj);
}

void post_peaq_did_read_request(char *public_key) {
    // Build request URL
    char read_request_url[256];
    snprintf(read_request_url, sizeof(read_request_url), "%s%s", config.main_api, PEAQ_DID_READ_ENDPOINT);

    // Build body
    char body[512];
    struct json_object *jobj = json_object_new_object();
    struct json_object *public_key_json = json_object_new_string(public_key);
    json_object_object_add(jobj, "peaq_public_key", public_key_json);
    const char *request_body = json_object_to_json_string(jobj);
    console(CONSOLE_DEBUG, "JSON body created: %s", request_body);

    // Build request options
    PostRequestOptions post_peaq_did_read_options = {
        .url = read_request_url,
        .key = access_key.public_key,
        .body = request_body,
        .filePath = NULL,
        .writeFunction = process_peaq_did_read_response,
        .writeData = NULL,
    };

    // Post
    performHttpPost(&post_peaq_did_read_options);

    // Free memory allocated to JSON object
    json_object_put(jobj);
}

void peaq_did_create_task() {
    // Check if the device is ready to generate a peaq_did
    // Mint complete and the proper chain (peaq) are a must!
    // if(state.access_status == 4 && state.chain == 1) {
    if (1) {
        console(CONSOLE_INFO, "Conditions met to generate peaq DID keys");
        generate_key_pair(PUBKEY_FILE_NAME, PRIVKEY_FILE_NAME);
        char *public_key = read_public_key();
        post_peaq_did_create_request(public_key);
    } else {
        console(CONSOLE_INFO, "Conditions not met for Peaq DID keys");
    }
}

void peaq_did_read_task() {
    // Ideally we do this only if a public key is already present
    // Should also have some indicator that the peaq_did_create_task has already happened
    console(CONSOLE_INFO, "Attempting to read PEAQ DID");
    char *public_key = read_public_key();
    post_peaq_did_read_request(public_key);
}
