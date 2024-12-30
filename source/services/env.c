#include "env.h"
#include "lib/console.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 256
#define MAX_VARIABLES 100

static Console csl = {
    .topic = "env",
};

typedef struct {
    char key[MAX_LINE_LENGTH];
    char value[MAX_LINE_LENGTH];
} EnvVariable;

EnvVariable env_variables[MAX_VARIABLES];
int env_variable_count = 0;

void load_env(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        print_debug(&csl, "failed to open .env file");
        return;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        // Delete the newline character
        line[strcspn(line, "\n")] = 0;

        // look for the equals sign
        char *equals_sign = strchr(line, '=');
        if (equals_sign == NULL) {
            continue; // invalid line
        }

        // separate the key and value
        size_t key_length = equals_sign - line;
        strncpy(env_variables[env_variable_count].key, line, key_length);
        env_variables[env_variable_count].key[key_length] = '\0';
        strcpy(env_variables[env_variable_count].value, equals_sign + 1);

        env_variable_count++;
        if (env_variable_count >= MAX_VARIABLES) {
            print_error(&csl, "too many environment variables");
            break;
        }
    }

    fclose(file);
}

const char *env(const char *key) {
    for (int i = 0; i < env_variable_count; i++) {
        if (strcmp(env_variables[i].key, key) == 0) {
            return env_variables[i].value;
        }
    }
    return NULL; // return NULL if the key is not found
}
