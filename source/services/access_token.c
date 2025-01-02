#include "access_token.h"
#include "lib/console.h"
#include "lib/scheduler.h"
#include "mosquitto.h"
#include "services/config.h"
#include <json-c/json.h>
#include <lib/http-requests.h>
#include <services/mqtt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ACCESS_TOKEN_ENDPOINT "access"
#define ACCESS_TOKEN_FILE "access-token.json"
#define ACCESS_TOKEN_EXPIRATION_MARGIN 3600

static Console csl = {
    .topic = "access token",
};

typedef struct {
    AccessToken *access_token;
    Registration *registration;
    struct mosquitto *mosq;
} AccessTokenTaskContext;

bool save_access_token(char *access_token_json) {
    char access_token_file_path[256];
    snprintf(access_token_file_path, sizeof(access_token_file_path), "%s/%s", config.data_path, ACCESS_TOKEN_FILE);

    FILE *file = fopen(access_token_file_path, "w");
    if (file == NULL) {
        print_error(&csl, "failed to open access token file for writing");
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
        print_debug(&csl, "could not open access token file; it might not exist yet");
        return NULL;
    }

    // Move to the end of the file to determine its size
    if (fseek(file, 0, SEEK_END) != 0) {
        print_debug(&csl, "failed to seek to the end of the access token file");
        fclose(file);
        return NULL;
    }

    long file_size_long = ftell(file);
    if (file_size_long < 0) {
        print_debug(&csl, "failed to get file size of access token file");
        fclose(file);
        return NULL;
    }

    // Ensure the file fits in size_t and prevent integer overflow
    if ((unsigned long)file_size_long + 1 > SIZE_MAX) {
        print_debug(&csl, "access token file is too large");
        fclose(file);
        return NULL;
    }

    size_t file_size = (size_t)file_size_long;

    // Reset the file position to the beginning
    if (fseek(file, 0, SEEK_SET) != 0) {
        print_debug(&csl, "failed to seek to the beginning of the access token file");
        fclose(file);
        return NULL;
    }

    // Allocate memory for the access token
    char *access_token = malloc(file_size + 1);
    if (access_token == NULL) {
        print_debug(&csl, "failed to allocate memory for access token");
        fclose(file);
        return NULL;
    }

    // Read the file's contents
    size_t bytes_read = fread(access_token, 1, file_size, file);
    if (bytes_read != file_size) {
        print_error(&csl, "failed to read access token file");
        free(access_token);
        fclose(file);
        return NULL;
    }

    access_token[file_size] = '\0';

    fclose(file);
    return access_token;
}

bool parse_access_token(const char *access_token_json, AccessToken *access_token) {
    access_token->token = NULL;
    access_token->issued_at_seconds = 0;
    access_token->expires_at_seconds = 0;

    json_object *json = json_tokener_parse(access_token_json);
    if (json == NULL) {
        print_error(&csl, "failed to parse access token json");
        return access_token;
    }

    json_object *token_json = NULL;
    if (!json_object_object_get_ex(json, "token", &token_json)) {
        print_error(&csl, "failed to get token from access token json");
        json_object_put(json);
        return false;
    }

    const char *token_str = json_object_get_string(token_json);
    if (token_str == NULL) {
        print_error(&csl, "failed to get token string from access token json");
        json_object_put(json);
        return false;
    }

    access_token->token = strdup(token_str);
    if (access_token->token == NULL) {
        print_error(&csl, "failed to allocate memory for access token string");
        json_object_put(json);
        return false;
    }

    json_object *issued_at_json = NULL;
    if (!json_object_object_get_ex(json, "issued_at_seconds", &issued_at_json)) {
        print_error(&csl, "failed to get issued_at_seconds from access token json");
        free(access_token->token);
        access_token->token = NULL;
        json_object_put(json);
        return false;
    }

    if (!json_object_is_type(issued_at_json, json_type_int)) {
        print_error(&csl, "issued_at_seconds is not an integer");
        free(access_token->token);
        access_token->token = NULL;
        json_object_put(json);
        return false;
    }

    access_token->issued_at_seconds = json_object_get_int64(issued_at_json);

    json_object *expires_at_json = NULL;
    if (!json_object_object_get_ex(json, "expires_at_seconds", &expires_at_json)) {
        print_error(&csl, "failed to get expires_at_seconds from access token json");
        free(access_token->token);
        access_token->token = NULL;
        json_object_put(json);
        return false;
    }

    if (!json_object_is_type(expires_at_json, json_type_int)) {
        print_error(&csl, "expires_at_seconds is not an integer");
        free(access_token->token);
        access_token->token = NULL;
        json_object_put(json);
        return false;
    }

    access_token->expires_at_seconds = json_object_get_int64(expires_at_json);

    json_object_put(json);
    return true;
}

char *request_access_token(Registration *registration) {
    char url[256];
    snprintf(url, sizeof(url), "%s/%s", config.accounting_api, ACCESS_TOKEN_ENDPOINT);

    // Convert registration to json
    json_object *json_body = json_object_new_object();
    json_object_object_add(json_body, "wayru_device_id", json_object_new_string(registration->wayru_device_id));
    json_object_object_add(json_body, "access_key", json_object_new_string(registration->access_key));
    const char *body_json_str = json_object_to_json_string(json_body);
    print_debug(&csl, "access request body is %s", body_json_str);

    HttpPostOptions options = {
        .url = url,
        .body_json_str = body_json_str,
    };

    HttpResult result = http_post(&options);
    json_object_put(json_body);

    if (result.is_error) {
        print_error(&csl, "failed to request access token with error: %s", result.error);
        return NULL;
    }

    if (result.response_buffer == NULL) {
        print_error(&csl, "no access token data found in response");
        return NULL;
    }

    return result.response_buffer;
}

time_t calculate_next_run(time_t expires_at_seconds, time_t access_interval) {
    time_t now = time(NULL);
    time_t next_run = expires_at_seconds - ACCESS_TOKEN_EXPIRATION_MARGIN;

    // Check if the token has already expired or is about to expire
    if (next_run <= now) return now;

    // Check if the next access interval is sooner than the expiration time
    time_t next_interval = now + access_interval;
    if (next_interval < next_run) return next_interval;

    // Default to the expiration time
    return next_run;
}

AccessToken *init_access_token(Registration *registration) {
    AccessToken *access_token = (AccessToken *)malloc(sizeof(AccessToken));
    if (access_token == NULL) {
        print_error(&csl, "failed to allocate memory for access token");
        return NULL;
    }

    // Initialize access token (default values)
    access_token->token = NULL;
    access_token->issued_at_seconds = 0;
    access_token->expires_at_seconds = 0;

    // Try to read the access token and check if it's still valid
    char *saved_access_token = read_access_token();
    if (saved_access_token != NULL) {
        bool parse_result = parse_access_token(saved_access_token, access_token);
        free(saved_access_token);
        if (parse_result) {
            if (time(NULL) < access_token->expires_at_seconds - ACCESS_TOKEN_EXPIRATION_MARGIN) {
                return access_token;
            }
        }
    }

    // Request a new access token
    char *access_token_json_str = request_access_token(registration);
    if (access_token_json_str == NULL) {
        print_error(&csl, "failed to request access token");
        return access_token;
    }

    bool save_result = save_access_token(access_token_json_str);
    if (!save_result) {
        print_error(&csl, "failed to save access token");
        free(access_token_json_str);
        return access_token;
    }

    bool parse_result = parse_access_token(access_token_json_str, access_token);
    if (!parse_result) {
        print_error(&csl, "failed to parse access token");
        free(access_token_json_str);
        return access_token;
    }

    print_info(&csl, "access token initialized");
    free(access_token_json_str);
    return access_token;
}

void access_token_task(Scheduler *sch, void *task_context) {
    AccessTokenTaskContext *context = (AccessTokenTaskContext *)task_context;

    char *access_token_json_str = request_access_token(context->registration);
    if (access_token_json_str == NULL) {
        print_error(&csl, "failed to request access token");
        return;
    }

    bool save_result = save_access_token(access_token_json_str);
    if (!save_result) {
        print_error(&csl, "failed to save access token");
        free(access_token_json_str);
        return;
    }

    bool parse_result = parse_access_token(access_token_json_str, context->access_token);
    if (!parse_result) {
        print_error(&csl, "failed to parse access token");
        free(access_token_json_str);
        return;
    }

    free(access_token_json_str);

    // Refresh mosquitto client
    refresh_mosquitto_access_token(context->mosq, context->access_token);

    // Schedule the task
    time_t next_run = calculate_next_run(context->access_token->expires_at_seconds, config.access_interval);
    schedule_task(sch, next_run, access_token_task, "access token task", context);
}

void access_token_service(Scheduler *sch,
                          AccessToken *access_token,
                          Registration *registration,
                          struct mosquitto *mosq) {
    AccessTokenTaskContext *context = (AccessTokenTaskContext *)malloc(sizeof(AccessTokenTaskContext));
    if (context == NULL) {
        print_error(&csl, "failed to allocate memory for access token task context");
        return;
    }

    context->access_token = access_token;
    context->registration = registration;
    context->mosq = mosq;

    // Schedule the task
    time_t next_run = calculate_next_run(access_token->expires_at_seconds, config.access_interval);
    schedule_task(sch, next_run, access_token_task, "access token task", context);
}

void clean_access_token(AccessToken *access_token) {
    if (access_token != NULL) {
        if (access_token->token != NULL) {
            free(access_token->token);
        }

        free(access_token);
    }
}

bool is_token_valid(const AccessToken *access_token) {
    if (access_token == NULL || access_token->token == NULL) {
        print_error(&csl, "Invalid access token object or token is NULL");
        return false;
    }

    time_t current_time = time(NULL);
    if (current_time == ((time_t)-1)) {
        print_error(&csl, "Failed to get the current time");
        return false;
    }

        print_debug(&csl, "Current time: %ld, Expires at: %ld", current_time, access_token->expires_at_seconds);

    if (current_time >= access_token->expires_at_seconds) {
        print_debug(&csl, "Access token has expired");
        return false;
    }

    print_debug(&csl, "Access token is valid");
    return true;
}

void validate_or_exit(const AccessToken *access_token) {
    if (!is_token_valid(access_token)) {
        print_error(&csl, "Access token is invalid. Closing wayru-os-services.");
        ///Cerrar proceso
    }
}
