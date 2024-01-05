#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "script_runner.h"

#define MIN_OUTPUT_SIZE 512

void run_script_and_save_output(const char *script_path, const char *output_path) {
    char buffer[1024];  // Buffer for reading script output
    FILE *script_pipe, *output_file;

    // Open the script for execution
    script_pipe = popen(script_path, "r");
    if (script_pipe == NULL) {
        perror("Failed to run script");
        return;
    }

    // Open the output file for writing
    output_file = fopen(output_path, "w");
    if (output_file == NULL) {
        perror("Failed to open output file for writing");
        pclose(script_pipe);
        return;
    }

    // Read the output of the script and write it to the output file
    while (fgets(buffer, sizeof(buffer), script_pipe) != NULL) {
        fputs(buffer, output_file);
    }

    // Close the script pipe and the output file
    pclose(script_pipe);
    fclose(output_file);

    printf("Script executed successfully, output saved to: %s\n", output_path);
}

char* run_script(const char* script_path) {
    char buffer[128];
    size_t result_size = MIN_OUTPUT_SIZE;
    char *result = (char *)malloc(result_size);
    if (result == NULL) {
        perror("[run_script] initial malloc failed");
        return NULL;
    }

    // Initialize the result string with a null character
    result[0] = '\0';

    FILE *pipe = popen(script_path, "r");
    if (!pipe) {
        perror("[run_script] popen failed");
        free(result);
        return NULL;
    }

    size_t current_result_length = 0;
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        // Make sure we have enough space in the result string, and reallocate if needed
        size_t needed_size = current_result_length + strlen(buffer) + 1;
        if (needed_size > result_size) {
            result_size = needed_size;
            char *new_result = realloc(result, result_size);
            if (new_result == NULL) {
                perror("[run_script] realloc failed");
                free(result);
                pclose(pipe);
                return NULL;
            }

            result = new_result;
        }
        strcat(result, buffer);
        current_result_length += strlen(buffer);
    }

    if (pclose(pipe) == -1) {
        perror("[run_script] error reported by pclose");
    }

    printf("[run_script] length of result: %zu\n", strlen(result));

    return result;
}