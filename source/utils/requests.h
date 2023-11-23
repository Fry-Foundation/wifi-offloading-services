#ifndef HTTP_REQUESTS_H
#define HTTP_REQUESTS_H

// Realiza una solicitud HTTP GET
int performHttpGet(const char *url, const char *filePath);

// Realiza una solicitud HTTP POST
typedef struct {
    const char *url;
    const char *key;
    const char *body;
    const char *filePath;
} PostRequestOptions;

int performHttpPost(const PostRequestOptions *options);

#endif /* HTTP_REQUESTS_H */