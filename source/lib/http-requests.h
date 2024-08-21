#ifndef HTTP_REQUESTS_H
#define HTTP_REQUESTS_H

#include <stdbool.h>

typedef struct {
    bool is_error;
    char *error;
    char *response_buffer;
} HttpResult;

typedef struct {
    const char *url;
    const char *legacy_key;
    const char *bearer_token;
} HttpGetOptions;

HttpResult http_get(const HttpGetOptions *options);

typedef struct {
    const char *url;
    const char *legacy_key;
    const char *bearer_token;
    const char *body_json_str;
    const char *upload_file_path;
} HttpPostOptions;

HttpResult http_post(const HttpPostOptions *options);

typedef struct {
    const char *url;
    const char *download_path;
} HttpDownloadOptions;

HttpResult http_download(const HttpDownloadOptions *options);

#endif /* HTTP_REQUESTS_H */
