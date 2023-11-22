#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "generate_id.h"
#include "base64.h"

char* generateId(const char* mac, const char* model) {
    // Calculate the length of the combined string
    int combinedLength = strlen(mac) + strlen(model) + 2; // +1 for hyphen, +1 for null terminator
    char *combined = (char *)malloc(combinedLength);
    if (combined == NULL) {
        perror("Failed to allocate memory for combined string");
        return NULL;
    }

    // Concatenate MAC and model with a hyphen
    snprintf(combined, combinedLength, "%s-%s", mac, model);

    // Calculate the length of the encoded string
    int encodedLength = Base64encode_len(combinedLength);

    // Allocate memory for the encoded string
    char *encoded = (char *)malloc(encodedLength);
    if (encoded == NULL) {
        perror("Failed to allocate memory for encoded data");
        free(combined);
        return NULL;
    }

    // Encode the combined string
    Base64encode(encoded, combined, combinedLength - 1); // -1 to exclude null terminator

    free(combined);
    return encoded;    
}