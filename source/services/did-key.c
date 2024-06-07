#include "did-key.h"
#include "lib/console.h"
#include "lib/key_pair.h"
#include "lib/requests.h"
#include "services/access.h"
#include "services/config.h"
#include <json-c/json.h>
#include <stdbool.h>

#define PRIVKEY_FILE_NAME "did_key"
#define PUBKEY_FILE_NAME "did_key.pub"

char *read_did_private_key() {
    console(CONSOLE_DEBUG, "reading did private key");

    char privkey_file_path[256];
    snprintf(privkey_file_path, sizeof(privkey_file_path), "%s/%s", config.data_path, PRIVKEY_FILE_NAME);

    FILE *file = fopen(privkey_file_path, "r");
    if (file == NULL) {
        console(CONSOLE_ERROR, "failed to open did privkey file");
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

char *read_did_public_key() {
    // 1. Open public key file
    // 2. Read file contents

    console(CONSOLE_DEBUG, "reading did public key");

    char pubkey_file_path[256];
    snprintf(pubkey_file_path, sizeof(pubkey_file_path), "%s/%s", config.data_path, PUBKEY_FILE_NAME);

    FILE *file = fopen(pubkey_file_path, "r");
    if (file == NULL) {
        console(CONSOLE_ERROR, "failed to open did public key file");
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

void create_did_key_pair() { generate_key_pair(PUBKEY_FILE_NAME, PRIVKEY_FILE_NAME); }