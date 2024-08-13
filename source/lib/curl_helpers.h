#ifndef CURL_HELPERS_H
#define CURL_HELPERS_H

#include <stdio.h>

char *init_response_buffer();

size_t save_to_buffer_callback(void *contents, size_t size, size_t nmemb, void *userp);

size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp);

#endif /* CURL_HELPERS_H */
