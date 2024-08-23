#ifndef HTTP_REQUESTS_H
#define HTTP_REQUESTS_H

#include <stdbool.h>
#include <stdio.h>

typedef struct {
    bool is_error;
    char *error;
    char *response_buffer;
    size_t response_size;
    double upload_speed_mbps;
    double download_speed_mbps;
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
    char *upload_data;
    size_t upload_data_size;
} HttpPostOptions;

HttpResult http_post(const HttpPostOptions *options);

typedef struct {
    const char *url;
    const char *download_path;
} HttpDownloadOptions;

HttpResult http_download(const HttpDownloadOptions *options);

#endif /* HTTP_REQUESTS_H */
