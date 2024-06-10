#include "did-key.h"
#include "lib/console.h"
#include "lib/key_pair.h"
#include "lib/requests.h"
#include "services/access.h"
#include "services/config.h"
#include <json-c/json.h>
#include <stdbool.h>
#include <string.h>

#define PRIVKEY_FILE_NAME "did_key"
#define PUBKEY_FILE_NAME "did_key.pub"
#define HEADER "-----BEGIN PUBLIC KEY-----"
#define FOOTER "-----END PUBLIC KEY-----"

char *read_did_private_key() {
    console(CONSOLE_DEBUG, "reading did private key");

    char privkey_file_path[256];
    snprintf(privkey_file_path, sizeof(privkey_file_path), "%s/%s", config.data_path, PRIVKEY_FILE_NAME);

    FILE *file = fopen(privkey_file_path, "r");
    if (file == NULL) {
        console(CONSOLE_ERROR, "failed to open did privkey file");
        return NULL;
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

    file_data_str[fsize] = '\0';

    // Find the key file headers to remove them later
    char *header_pos = strstr(file_data_str, HEADER);
    char *footer_pos = strstr(file_data_str, FOOTER);

    if (header_pos == NULL || footer_pos == NULL) {
        console(CONSOLE_ERROR, "Invalid key file format");
        free(file_data_str);
        return 0;
    }

    // Move past the header and newlines
    char *key_start = header_pos + strlen(HEADER);
    while (*key_start == '\n' || *key_start == '\r') {
        key_start++;
    }

    // Move to the position of the footer and backup to remove newlines
    char *key_end = footer_pos;
    while (key_end > key_start && (*(key_end - 1) == '\n' || *(key_end - 1) == '\r')) {
        key_end--;
    }

    size_t key_length = key_end - key_start;

    char *key_content = malloc(key_length + 1);
    strncpy(key_content, key_start, key_length);
    key_content[key_length] = '\0';

    free(file_data_str);

    console(CONSOLE_DEBUG, key_content);
    return key_content;
}

void create_did_key_pair() { generate_key_pair(PUBKEY_FILE_NAME, PRIVKEY_FILE_NAME); }