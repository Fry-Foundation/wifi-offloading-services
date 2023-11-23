#include <stdio.h>
#include <curl/curl.h>
#include "requests.h"

// HTTP GET request
int performHttpGet(const char *url, const char *filePath)
{
    CURL *curl;
    CURLcode res = CURLE_OK;

    curl = curl_easy_init();
    if (curl)
    {
        FILE *file = fopen(filePath, "wb"); // Abrir el archivo en modo de escritura binaria
        if (!file)
        {
            fprintf(stderr, "Error al abrir el archivo para escritura.\n");
            return -1;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);

        // Guardar la respuesta en el archivo
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            fclose(file); // Cerrar el archivo si hubo un error
            return -1;
        }

        curl_easy_cleanup(curl);
        fclose(file); // Cerrar el archivo después de la operación exitosa
    }
    else
    {
        fprintf(stderr, "Error initializing curl\n");
        return -1;
    }

    return 0;
}

// HTTP POST request
int performHttpPost(const PostRequestOptions *options)
{
    CURL *curl;
    CURLcode res = CURLE_OK;

    curl = curl_easy_init();
    if (curl)
    {
        FILE *file;
        struct curl_slist *headers = NULL;

        // Options
        curl_easy_setopt(curl, CURLOPT_URL, options->url);

        if (options->key != NULL) {
            char keyHeader[255];
            snprintf(keyHeader, 255, "public_key: %s", options->key);
            headers = curl_slist_append(headers, keyHeader);
        } else {
            printf("Skipping key\n");
        }  

        if (options->body != NULL) {
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, options->body);
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);        

        if(options->filePath != NULL) {
            file = fopen(options->filePath, "wb");
            if (!file)
            {
                fprintf(stderr, "Error al abrir el archivo para escritura.\n");
                return -1;
            }

            curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        }

        // Request
        res = curl_easy_perform(curl);

        // Response
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        // Cleanup
        if(options->filePath != NULL) {
            fclose(file);
        }
        curl_slist_free_all(headers); // Free the header list
        curl_easy_cleanup(curl);

        // res = curl_easy_perform(curl);
        // printf("res %d\n", res);

        // if (res != CURLE_OK)
        // {
        //     fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        //     return -1;
        // }

        // printf("Done\n");

        // curl_easy_cleanup(curl);
    }
    else
    {
        fprintf(stderr, "Error initializing curl\n");
        return -1;
    }

    return 0;
}
