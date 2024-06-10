#include "requests.h"
#include "console.h"
#include <curl/curl.h>
#include <stdio.h>

// HTTP GET request
int performHttpGet(const char *url, const char *file_path) {
    CURL *curl;
    CURLcode res = CURLE_OK;

    curl = curl_easy_init();
    if (curl) {
        FILE *file = fopen(file_path, "wb"); // Abrir el archivo en modo de escritura binaria
        if (!file) {
            console(CONSOLE_ERROR, "failed to open file for writing %s", file_path);
            return -1;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);

        // Guardar la respuesta en el archivo
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            console(CONSOLE_ERROR, "curl GET failed: %s", curl_easy_strerror(res));
            fclose(file);
            return -1;
        }

        curl_easy_cleanup(curl);
        fclose(file);
    } else {
        console(CONSOLE_ERROR, "curl did not initialize");
        return -1;
    }

    return 1;
}

// HTTP POST request
int performHttpPost(const PostRequestOptions *options) {
    CURL *curl;
    CURLcode res = CURLE_OK;

    curl = curl_easy_init();
    if (curl) {
        FILE *file;
        struct curl_slist *headers = NULL;

        // Options
        curl_easy_setopt(curl, CURLOPT_URL, options->url);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        if (options->key != NULL) {
            char keyHeader[1024];
            snprintf(keyHeader, 1024, "public_key: %s", options->key);
            headers = curl_slist_append(headers, keyHeader);
        }

        if (options->body != NULL) {
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, options->body);
        } else {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        if (options->filePath != NULL) {
            file = fopen(options->filePath, "wb");
            if (!file) {
                console(CONSOLE_ERROR, "failed to open the file for writing curl POST response");
                return -1;
            }

            curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        }

        if (options->writeFunction != NULL) {
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, options->writeFunction);
        }

        if (options->writeData != NULL) {
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, options->writeData);
        }

        // Request
        res = curl_easy_perform(curl);
        console(CONSOLE_DEBUG, "response code: %d", res);

        // Response
        if (res != CURLE_OK) {
            console(CONSOLE_ERROR, "curl POST failed: %s", curl_easy_strerror(res));
        }

        // Cleanup
        if (options->filePath != NULL) {
            fclose(file);
        }
        curl_slist_free_all(headers); // Free the header list
        curl_easy_cleanup(curl);
    } else {
        console(CONSOLE_ERROR, "curl did not initialize");
        return -1;
    }

    return 1;
}
