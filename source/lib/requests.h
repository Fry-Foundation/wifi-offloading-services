#ifndef HTTP_REQUESTS_H
#define HTTP_REQUESTS_H

#include <stdio.h>

// Realiza una solicitud HTTP GET
int performHttpGet(const char *url, const char *filePath);

// Realiza una solicitud HTTP POST
typedef struct {
    const char *url;
    const char *key;
    const char *body;
    const char *filePath;
    const char *resultFilePath;
    size_t (*writeFunction)(char *ptr, size_t size, size_t nmemb, void *userdata);
    void *writeData;
} PostRequestOptions;

int performHttpPost(const PostRequestOptions *options);

#endif /* HTTP_REQUESTS_H */
