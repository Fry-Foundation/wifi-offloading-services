#include "lib/console.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/http-requests.h"

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
    HttpResult *result = (HttpResult *)userp;

    // Allocate memory to store the response
    char *temp = realloc(result->response_buffer, result->response_size + total_size + 1);
    if (temp == NULL) {
        fprintf(stderr, "realloc() failed\n");
        free(result->response_buffer); // Free the original memory to prevent memory leaks
        result->response_buffer = NULL;
        result->response_size = 0;
        return 0;
    }

    result->response_buffer = temp;

    // Append the new data to the response buffer
    memcpy(result->response_buffer + result->response_size, contents, total_size);
    result->response_size += total_size;
    result->response_buffer[result->response_size] = '\0'; // Null-terminate the string

    return total_size;
}

size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp) {
    HttpPostOptions *options = (HttpPostOptions *)userp;
    size_t total_size = size * nmemb;
    size_t bytes_to_copy = (total_size > options->upload_data_size) ? options->upload_data_size : total_size;

    memcpy(ptr, options->upload_data, bytes_to_copy);
    options->upload_data += bytes_to_copy;
    options->upload_data_size -= bytes_to_copy;

    return bytes_to_copy;
}