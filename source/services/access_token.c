#include "access_token.h"
#include "lib/console.h"
#include "lib/scheduler.h"
#include "services/config.h"
#include <json-c/json.h>
#include <lib/http-requests.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ACCESS_TOKEN_ENDPOINT "access"
#define ACCESS_TOKEN_FILE "access-token.json"

static AccessToken *access_token;
static Registration *registration;

bool save_access_token(char *access_token_json) {
    char access_token_file_path[256];
    snprintf(access_token_file_path, sizeof(access_token_file_path), "%s/%s", config.data_path, ACCESS_TOKEN_FILE);

    FILE *file = fopen(access_token_file_path, "w");
    if (file == NULL) {
        console(CONSOLE_ERROR, "failed to open access token file");
        return false;
    }

    fprintf(file, "%s", access_token_json);
    fclose(file);

    return true;
}

char *read_access_token() {
    char access_token_file_path[256];
    snprintf(access_token_file_path, sizeof(access_token_file_path), "%s/%s", config.data_path, ACCESS_TOKEN_FILE);

    FILE *file = fopen(access_token_file_path, "r");
    if (file == NULL) {
        console(CONSOLE_ERROR, "failed to open access token file");
        return NULL;
    }

    // Get file size to allocate memory
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *access_token = malloc(file_size + 1);
    if (access_token == NULL) {
        console(CONSOLE_ERROR, "failed to allocate memory for access token");
        fclose(file);
        return NULL;
    }

    // Read file
    fread(access_token, 1, file_size, file);
    access_token[file_size] = '\0';
    fclose(file);

    return access_token;
}

AccessToken parse_access_token(const char *access_token_json) {
    AccessToken access_token;
    access_token.token = NULL;
    access_token.issued_at_seconds = 0;
    access_token.expires_at_seconds = 0;

    json_object *json = json_tokener_parse(access_token_json);
    if (json == NULL) {
        console(CONSOLE_ERROR, "failed to parse access token json");
        return access_token;
    }

    json_object *token_json = NULL;
    if (!json_object_object_get_ex(json, "token", &token_json)) {
        console(CONSOLE_ERROR, "failed to get token from access token json");
        json_object_put(json);
        return access_token;
    }

    access_token.token = strdup(json_object_get_string(token_json));

    json_object *issued_at_json = NULL;
    if (!json_object_object_get_ex(json, "issued_at_seconds", &issued_at_json)) {
        console(CONSOLE_ERROR, "failed to get issued_at from access token json");
        json_object_put(json);
        return access_token;
    }

    access_token.issued_at_seconds = json_object_get_int64(issued_at_json);

    json_object *expires_at_json = NULL;
    if (!json_object_object_get_ex(json, "expires_at_seconds", &expires_at_json)) {
        console(CONSOLE_ERROR, "failed to get expires_at from access token json");
        json_object_put(json);
        return access_token;
    }

    access_token.expires_at_seconds = json_object_get_int64(expires_at_json);

    json_object_put(json);

    return access_token;
}

char *request_access_token(Registration *_registration) {
    char url[256];
    snprintf(url, sizeof(url), "%s/%s", config.accounting_api, ACCESS_TOKEN_ENDPOINT);

    // Convert registration to json
    json_object *json_body = json_object_new_object();
    json_object_object_add(json_body, "wayru_device_id", json_object_new_string(_registration->wayru_device_id));
    json_object_object_add(json_body, "access_key", json_object_new_string(_registration->access_key));
    const char *body_json_str = json_object_to_json_string(json_body);
    console(CONSOLE_DEBUG, "access request body is %s", body_json_str);

    HttpPostOptions options = {
        .url = url,
        .body_json_str = body_json_str,
    };

    HttpResult result = http_post(&options);
    json_object_put(json_body);

    if (result.is_error) {
        console(CONSOLE_ERROR, "failed to request access token");
        console(CONSOLE_ERROR, "error: %s", result.error);
        return NULL;
    }

    if (result.response_buffer == NULL) {
        console(CONSOLE_ERROR, "no access token data found in response");
        return NULL;
    }

    return result.response_buffer;
}

AccessToken *init_access_token(Registration *_registration) {
    AccessToken *access_token = (AccessToken *)malloc(sizeof(AccessToken));
    if (access_token != NULL) {
        access_token->token = NULL;
        access_token->issued_at_seconds = 0;
        access_token->expires_at_seconds = 0;
    }

    if (read_access_token()) {
        AccessToken parsed_access_token = parse_access_token(read_access_token());
        if (parsed_access_token.token != NULL && parsed_access_token.issued_at_seconds != 0 &&
            parsed_access_token.expires_at_seconds != 0) {
            access_token->token = parsed_access_token.token;
            access_token->issued_at_seconds = parsed_access_token.issued_at_seconds;
            access_token->expires_at_seconds = parsed_access_token.expires_at_seconds;
            console(CONSOLE_DEBUG, "access token is already available");
            console(CONSOLE_DEBUG, "token: %s", access_token->token);
            return access_token;
        }
    }

    char *access_token_json = request_access_token(_registration);
    if (access_token_json == NULL) {
        console(CONSOLE_ERROR, "failed to request access token");
        return access_token;
    }

    if (!save_access_token(access_token_json)) {
        console(CONSOLE_ERROR, "failed to save access token");
        return access_token;
    }

    AccessToken parsed_access_token = parse_access_token(access_token_json);
    if (parsed_access_token.token == NULL) {
        console(CONSOLE_ERROR, "failed to parse access token");
        return access_token;
    }

    free(access_token_json);

    access_token->token = parsed_access_token.token;
    access_token->issued_at_seconds = parsed_access_token.issued_at_seconds;
    access_token->expires_at_seconds = parsed_access_token.expires_at_seconds;
    console(CONSOLE_DEBUG, "token: %s", access_token->token);
    return access_token;
}

void access_token_task(Scheduler *sch) {
    char *access_token_json = request_access_token(registration);
    if (access_token_json == NULL) {
        console(CONSOLE_ERROR, "failed to request access token");
        return;
    }

    if (!save_access_token(access_token_json)) {
        console(CONSOLE_ERROR, "failed to save access token");
        return;
    }

    AccessToken parsed_access_token = parse_access_token(access_token_json);
    if (parsed_access_token.token == NULL) {
        console(CONSOLE_ERROR, "failed to parse access token");
        return;
    }

    free(access_token_json);
    access_token->token = parsed_access_token.token;
    access_token->issued_at_seconds = parsed_access_token.issued_at_seconds;
    access_token->expires_at_seconds = parsed_access_token.expires_at_seconds;
    console(CONSOLE_DEBUG, "token: %s", access_token->token);
    console(CONSOLE_DEBUG, "issued at seconds: %ld", access_token->issued_at_seconds);
    console(CONSOLE_DEBUG, "expires at seconds: %ld", access_token->expires_at_seconds);

    schedule_task(sch, time(NULL) + config.accounting_interval, access_token_task, "access token task");
}

void access_token_service(Scheduler *sch, AccessToken *_access_token, Registration *_registration) {
    access_token = _access_token;
    registration = _registration;
    access_token_task(sch);
}

void clean_access_token(AccessToken *access_token) {
    if (access_token != NULL) {
        if (access_token->token != NULL) {
            free(access_token->token);
        }

        free(access_token);
    }
}
