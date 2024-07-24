#include "lib/console.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *init_response_buffer() {
    char *response = malloc(1);
    if (response == NULL) {
        fprintf(stderr, "malloc() failed\n");
        return NULL;
    }

    response[0] = '\0';
    return response;
}

size_t save_to_buffer_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total_size = size * nmemb;
    char **response_ptr = (char **)userp;
    size_t current_length = strlen(*response_ptr);

    // Allocate memory to store the response
    char *temp = realloc(*response_ptr, current_length + total_size + 1);
    if (temp == NULL) {
        fprintf(stderr, "realloc() failed\n");
        free(*response_ptr); // Free the original memory to prevent memory leaks
        *response_ptr = NULL;
        return 0;
    }

    *response_ptr = temp;

    // Append the new data to the response buffer
    memcpy(*response_ptr + current_length, contents, total_size);
    (*response_ptr)[current_length + total_size] = '\0'; // Null-terminate the string

    return total_size;
}
