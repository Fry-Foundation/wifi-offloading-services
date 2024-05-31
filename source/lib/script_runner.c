#include "script_runner.h"
#include "console.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_OUTPUT_SIZE 512
#define MAX_COMMAND_SIZE 256

void run_script_and_save_output(const char *script_path, const char *output_path) {
    char buffer[1024]; // Buffer for reading script output
    FILE *script_pipe, *output_file;

    // Open the script for execution
    script_pipe = popen(script_path, "r");
    if (script_pipe == NULL) {
        console(CONSOLE_ERROR, "failed to run script %s", script_path);
        return;
    }

    // Open the output file for writing
    output_file = fopen(output_path, "w");
    if (output_file == NULL) {
        console(CONSOLE_ERROR, "failed to open file for writing %s", output_path);
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

    console(CONSOLE_DEBUG, "script executed successfully, output saved to: %s", output_path);
}

// Make sure to free the char* returned by this function
char *run_script(const char *script_path) {
    char buffer[128];
    size_t result_size = MIN_OUTPUT_SIZE;
    char *result = (char *)malloc(result_size);
    if (result == NULL) {
        console(CONSOLE_ERROR, "memory allocation for script result failed");
        return NULL;
    }

    // Initialize the result string with a null character
    result[0] = '\0';

    // Create a command that redirects stderr
    char command[MAX_COMMAND_SIZE];
    snprintf(command, sizeof(command), "%s 2>&1", script_path);

    FILE *pipe = popen(command, "r");
    if (!pipe) {
        console(CONSOLE_ERROR, "failed to run script %s", script_path);
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
                console(CONSOLE_ERROR, "script result reallocation failed for %s", script_path);
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
        console(CONSOLE_ERROR, "pclose reported exit code -1");
    }

    console(CONSOLE_DEBUG, "length of result: %zu", strlen(result));

    return result;
}