#ifndef HTTP_REQUESTS_H
#define HTTP_REQUESTS_H

// Realiza una solicitud HTTP GET
int performHttpGet(const char *url, const char *filePath);

// Realiza una solicitud HTTP POST
int performHttpPost(const char *url, const char *postData);

#endif /* HTTP_REQUESTS_H */