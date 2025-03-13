#include "http-requests.h"
#include "lib/console.h"
#include "lib/curl_helpers.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Console csl = {
    .topic = "http-requests",
};

// HTTP GET request
HttpResult http_get(const HttpGetOptions *options) {
    HttpResult result = {
        .is_error = false,
        .error = NULL,
        .http_status_code = 0,
        .response_buffer = NULL,
        .response_size = 0,
    };

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        print_error(&csl, "curl did not initialize");
        result.is_error = true;
        result.error = "curl did not initialize";
        return result;
    }

    CURLcode res = CURLE_OK;
    struct curl_slist *headers = NULL;

    char *response_buffer = init_response_buffer();
    if (response_buffer == NULL) {
        result.is_error = true;
        result.error = "failed to initialize response buffer";
        return result;
    }

    // CURL Options
    curl_easy_setopt(curl, CURLOPT_URL, options->url);

    if (options->legacy_key != NULL) {
        char legacy_key_header[1024];
        snprintf(legacy_key_header, 1024, "public_key: %s", options->legacy_key);
        headers = curl_slist_append(headers, legacy_key_header);
    }

    if (options->bearer_token != NULL) {
        char auth_header[1024];
        snprintf(auth_header, 1024, "Authorization: Bearer %s", options->bearer_token);
        headers = curl_slist_append(headers, auth_header);
    }

    if (headers != NULL) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    // Response callback and buffer
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, save_to_buffer_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

    // Request
    res = curl_easy_perform(curl);
    print_debug(&csl, "response buffer: %s", result.response_buffer);

    // Response
    if (res != CURLE_OK) {
        print_error(&csl, "curl GET failed: %s", curl_easy_strerror(res));
        free(result.response_buffer);
        result.is_error = true;
        result.error = strdup(curl_easy_strerror(res));
    } else {
        print_debug(&csl, "response buffer: %s", result.response_buffer);

        // Get HTTP status code
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.http_status_code);
        print_debug(&csl, "HTTP status code: %ld", result.http_status_code);

        if (result.http_status_code >= 400) {
            print_error(&csl, "HTTP status code is greater than 400, error");
            result.is_error = true;
            result.error = strdup("HTTP error, check status code and response buffer");
        } else {
            result.is_error = false;
        }
    }

    if (headers != NULL) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return result;
}

// HTTP POST request
HttpResult http_post(const HttpPostOptions *options) {
    HttpResult result = {
        .is_error = false,
        .error = NULL,
        .http_status_code = 0,
        .response_buffer = NULL,
        .response_size = 0,
    };

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        print_error(&csl, "curl did not initialize");
        result.is_error = true;
        result.error = "curl did not initialize";
        return result;
    }

    CURLcode res = CURLE_OK;
    curl_mime *form = NULL;
    curl_mimepart *field = NULL;
    struct curl_slist *headers = NULL;

    char *response_buffer = init_response_buffer();
    if (response_buffer == NULL) {
        result.is_error = true;
        result.error = "failed to initialize response buffer";
        return result;
    }

    // CURL Options
    curl_easy_setopt(curl, CURLOPT_URL, options->url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    if (options->legacy_key != NULL) {
        char legacy_key_header[1024];
        snprintf(legacy_key_header, 1024, "public_key: %s", options->legacy_key);
        headers = curl_slist_append(headers, legacy_key_header);
    }

    if (options->bearer_token != NULL) {
        char auth_header[1024];
        snprintf(auth_header, 1024, "Authorization: Bearer %s", options->bearer_token);
        headers = curl_slist_append(headers, auth_header);
    }

    if (options->body_json_str != NULL) {
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, options->body_json_str);
    } else if (options->upload_data == NULL) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
    }

    if (headers != NULL) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    if (options->upload_file_path != NULL) {
        form = curl_mime_init(curl);
        field = curl_mime_addpart(form);
        curl_mime_name(field, "file");
        curl_mime_filedata(field, options->upload_file_path);
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
    }

    if (options->upload_data != NULL) {
        form = curl_mime_init(curl);
        field = curl_mime_addpart(form);
        curl_mime_name(field, "file");
        curl_mime_type(field, "application/octet-stream");
        curl_mime_data(field, options->upload_data, options->upload_data_size);
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, (void *)options);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, options->upload_data_size);
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, save_to_buffer_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

    // Request
    res = curl_easy_perform(curl);
    print_debug(&csl, "curl code: %d", res);

    // Response
    if (res != CURLE_OK) {
        print_error(&csl, "curl POST failed: %s", curl_easy_strerror(res));
        free(result.response_buffer);
        result.is_error = true;
        result.error = strdup(curl_easy_strerror(res));
    } else {
        print_debug(&csl, "response buffer: %s", result.response_buffer);

        // Get HTTP status code
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.http_status_code);
        print_debug(&csl, "HTTP status code: %ld", result.http_status_code);

        if (result.http_status_code >= 400) {
            print_error(&csl, "HTTP status code is greater than 400, error");
            result.is_error = true;
            result.error = strdup("HTTP error, check status code and response buffer");
        } else {
            result.is_error = false;
        }
    }

    // Cleanup
    if (form != NULL) curl_mime_free(form);
    if (headers != NULL) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return result;
}

// HTTP file download
HttpResult http_download(const HttpDownloadOptions *options) {
    CURL *curl;
    CURLcode res;
    FILE *fp;
    HttpResult result = {
        .is_error = false,
        .error = NULL,
        .http_status_code = 0,
        .response_buffer = NULL,
        .response_size = 0,
    };

    curl = curl_easy_init();
    if (!curl) {
        print_debug(&csl, "Failed to initialize curl");
        result.is_error = true;
        result.error = strdup("Failed to initialize curl");
        return result;
    }

    fp = fopen(options->download_path, "wb");
    if (!fp) {
        print_debug(&csl, "Failed to open file for writing");
        result.is_error = true;
        result.error = strdup("Failed to open file for writing");
        curl_easy_cleanup(curl);
        return result;
    }

    struct curl_slist *headers = NULL;
    if (options->bearer_token != NULL) {
        char auth_header[1024];
        snprintf(auth_header, 1024, "Authorization: Bearer %s", options->bearer_token);
        headers = curl_slist_append(headers, auth_header);
    }

    curl_easy_setopt(curl, CURLOPT_URL, options->url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    if (headers != NULL) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        print_debug(&csl, "Failed to perform curl request: %s", curl_easy_strerror(res));
        result.is_error = true;
        result.error = strdup(curl_easy_strerror(res));
    } else {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.http_status_code);
        if (result.http_status_code >= 400) {
            print_debug(&csl, "HTTP error, check status code and response buffer");
            result.is_error = true;
            result.error = strdup("HTTP error, check status code and response buffer");
        }
    }

    fclose(fp);
    curl_easy_cleanup(curl);
    return result;
}
