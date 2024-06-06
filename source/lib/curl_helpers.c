#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/console.h"

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

    // Allocate memory to store the response
    *response_ptr = realloc(*response_ptr, strlen(*response_ptr) + total_size + 1);

    if (*response_ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        return 0;
    }

    // Append the new data to the response buffer
    strncat(*response_ptr, contents, total_size);

    return total_size;
}