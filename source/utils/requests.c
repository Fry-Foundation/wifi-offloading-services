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
int performHttpPost(const char *url, const char *postData)
{
    CURL *curl;
    CURLcode res = CURLE_OK;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return -1;
        }

        curl_easy_cleanup(curl);
    }
    else
    {
        fprintf(stderr, "Error initializing curl\n");
        return -1;
    }

    return 0;
}
