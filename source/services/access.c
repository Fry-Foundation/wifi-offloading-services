#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <json-c/json.h>
#include "access.h"
#include "setup.h"
#include "accounting.h"
#include "../store/config.h"
#include "../store/state.h"
#include "../utils/requests.h"
#include "../utils/script_runner.h"

#define KEY_FILE "/data/access-key"
#define KEY_FILE_BUFFER_SIZE 768
#define REQUEST_BODY_BUFFER_SIZE 256
#define MAX_KEY_SIZE 512
#define MAX_TIMESTAMP_SIZE 256
// #define ACCESS_ENDPOINT "https://api.wayru.tech/api/nfnode/access"

time_t convertToTime_t(const char *timestampStr)
{
    long long int epoch = strtoll(timestampStr, NULL, 10);
    return (time_t)epoch;
}

AccessKey *initAccessKey()
{
    AccessKey *accessKey = malloc(sizeof(AccessKey));
    accessKey->key = NULL;
    accessKey->createdAt = 0;
    accessKey->expiresAt = 0;

    readAccessKey(accessKey);

    return accessKey;
}

int readAccessKey(AccessKey *accessKey)
{
    printf("[access] reading stored access key\n");

    char keyFile[KEY_FILE_BUFFER_SIZE];
    snprintf(keyFile, sizeof(keyFile), "%s%s", getConfig().basePath, KEY_FILE);

    FILE *file = fopen(keyFile, "r");
    if (file == NULL)
    {
        // Handle error (e.g., file not found)
        fprintf(stderr, "Failed to open key file.\n");
        return 0;
    }

    char line[512];
    char public_key[MAX_KEY_SIZE];
    char created_at[MAX_TIMESTAMP_SIZE];
    char expires_at[MAX_TIMESTAMP_SIZE];

    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "public_key", 10) == 0)
        {
            // Subtract the length of "public_key " from the total length
            size_t keyLength = strlen(line) - 11;
            accessKey->key = malloc(keyLength + 1);
            if (accessKey->key == NULL)
            {
                perror("Failed to allocate memory for key");
                fclose(file);
                return 0;
            }
            strcpy(accessKey->key, line + 11);
            accessKey->key[keyLength] = '\0';
        }
        else if (strncmp(line, "created_at", 10) == 0)
        {
            sscanf(line, "created_at %s", created_at);
        }
        else if (strncmp(line, "expires_at", 10) == 0)
        {
            sscanf(line, "expires_at %s", expires_at);
        }
    }

    fclose(file);

    time_t createdAt = convertToTime_t(created_at);
    time_t expiresAt = convertToTime_t(expires_at);

    accessKey->createdAt = createdAt;
    accessKey->expiresAt = expiresAt;

    return 1;
}

void writeAccessKey(AccessKey *accessKey)
{
    printf("[access] writing new access key\n");

    char keyFile[KEY_FILE_BUFFER_SIZE];
    snprintf(keyFile, sizeof(keyFile), "%s%s", getConfig().basePath, KEY_FILE);

    FILE *file = fopen(keyFile, "w");
    if (file == NULL)
    {
        printf("Unable to open file for writing\n");
        return;
    }

    fprintf(file, "public_key %s\n", accessKey->key);
    fprintf(file, "created_at %ld\n", accessKey->createdAt);
    fprintf(file, "expires_at %ld\n", accessKey->expiresAt);

    fclose(file);
}

void processAccessStatus(char *status)
{
    printf("[access] Processing access status\n");
    if (strcmp(status, "initial") == 0)
    {
        state.accessStatus = 0;
    }
    else if (strcmp(status, "banned") == 0)
    {
        state.accessStatus = 1;
    }
    else if (strcmp(status, "setup-pending") == 0)
    {
        state.accessStatus = 2;
    }
    else if (strcmp(status, "setup-approved") == 0)
    {
        state.accessStatus = 3;
    }
    else if (strcmp(status, "setup-completed") == 0)
    {
        state.accessStatus = 4;
    }
}

size_t processAccessKeyResponse(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t realsize = size * nmemb;
    AccessKey *accessKey = (AccessKey *)userdata;

    fprintf(stderr, "[access] Received JSON data: %s\n", ptr);

    // Parse JSON
    struct json_object *parsedResponse;
    struct json_object *publicKey;
    struct json_object *status;
    struct json_object *payload;
    struct json_object *iat;
    struct json_object *exp;

    parsedResponse = json_tokener_parse(ptr);
    if (parsedResponse == NULL)
    {
        // JSON parsing failed
        fprintf(stderr, "Failed to parse JSON\n");
        return realsize;
    }

    // Extract fields
    // Enhanced error logging
    bool errorOccurred = false;
    if (!json_object_object_get_ex(parsedResponse, "publicKey", &publicKey))
    {
        fprintf(stderr, "[access] error: publicKey field missing or invalid\n");
        errorOccurred = true;
    }
    if (!json_object_object_get_ex(parsedResponse, "status", &status))
    {
        fprintf(stderr, "[access] error: status field missing or invalid\n");
        errorOccurred = true;
    }
    if (!json_object_object_get_ex(parsedResponse, "payload", &payload))
    {
        fprintf(stderr, "[access] error: payload field missing or invalid\n");
        errorOccurred = true;
    }
    if (payload && !json_object_object_get_ex(payload, "iat", &iat))
    {
        fprintf(stderr, "[access] error: iat field missing or invalid in payload\n");
        errorOccurred = true;
    }
    if (payload && !json_object_object_get_ex(payload, "exp", &exp))
    {
        fprintf(stderr, "[access] error: exp field missing or invalid in payload\n");
        errorOccurred = true;
    }

    if (errorOccurred)
    {
        json_object_put(parsedResponse);
        return realsize;
    }

    accessKey->key = malloc(strlen(json_object_get_string(publicKey)) + 1); // +1 for null-terminator
    strcpy(accessKey->key, json_object_get_string(publicKey));
    accessKey->createdAt = json_object_get_int64(iat);
    accessKey->expiresAt = json_object_get_int64(exp);

    char *statusValue = malloc(strlen(json_object_get_string(status)) + 1);
    strcpy(statusValue, json_object_get_string(status));
    printf("[access] status: %s\n", statusValue);
    processAccessStatus(statusValue);

    json_object_put(parsedResponse);
    return realsize;
}

int checkAccessKeyExpiration(AccessKey *accessKey)
{
    printf("[access] Checking expiration\n");
    time_t now;
    time(&now);

    if (difftime(now, accessKey->expiresAt) > 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int checkAccessKeyNearExpiration(AccessKey *accessKey)
{
    printf("[access] Checking if key is near expiration\n");
    time_t now;
    time(&now);

    if (difftime(accessKey->expiresAt, now) <= 600)
    {
        printf("[access] Key is near expiration\n");
        return 1;
    }
    else
    {
        printf("[access] Key is not near expiration\n");
        return 0;
    }
}

int requestAccessKey(AccessKey *accessKey)
{
    printf("[access] Request access key\n");

    char keyFile[KEY_FILE_BUFFER_SIZE];
    snprintf(keyFile, sizeof(keyFile), "%s%s", getConfig().basePath, KEY_FILE);

    json_object *jsonData = json_object_new_object();

    json_object_object_add(jsonData, "device_id", json_object_new_string(getConfig().deviceId));
    json_object_object_add(jsonData, "mac", json_object_new_string(getConfig().mac));
    json_object_object_add(jsonData, "brand", json_object_new_string(getConfig().brand));
    json_object_object_add(jsonData, "model", json_object_new_string(getConfig().model));
    json_object_object_add(jsonData, "public_ip", json_object_new_string(getConfig().public_ip));
    json_object_object_add(jsonData, "os_name", json_object_new_string(getConfig().os_name));
    json_object_object_add(jsonData, "os_version", json_object_new_string(getConfig().osVersion));
    json_object_object_add(jsonData, "os_services_version", json_object_new_string(getConfig().servicesVersion));
    json_object_object_add(jsonData, "on_boot", json_object_new_string(state.onBoot == 1 ? "true" : "false"));

    const char *jsonDataString = json_object_to_json_string(jsonData);
    printf("[access] DeviceData -> %s\n", jsonDataString);

    //  Obtener MAIN API DE UCI
    FILE *fp;
    char buffer[256];
    const char *main_api = NULL;

    // Ejecutar el script de shell y capturar su salida
    fp = popen("/usr/sbin/conf.sh", "r");
    // fp = popen("/home/lmva/wayru-os-services/source/scripts/dev/conf.sh", "r");
    if (fp == NULL)
    {
        printf("Error al abrir conf.sh");
        return 1;
    }

    // Leer la salida del script línea por línea
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        char key[256], value[256];
        if (sscanf(buffer, "%[^=]=%s", key, value) == 2)
        {
            if (strcmp(key, "main_api") == 0)
            {
                // Actualizar el valor de main_api
                main_api = strdup(value); // Guardar una copia del valor
                break;
            }
        }
    }
    // Cerrar el proceso del script
    pclose(fp);

    // Utilizar el valor capturado en el programa C
    /*if (main_api != NULL)
    {
        printf("El valor de main_api obtenido del script es: %s\n", main_api);
        // Ahora puedes utilizar 'main_api' en tu programa C como desees

        // Por ejemplo, mostrar el valor en un mensaje
        printf("Usando main_api en alguna funcionalidad:\n");
        printf("main_api: %s\n", main_api);

        // Liberar memoria si es necesario
        free((void *)main_api);
    }
    else
    {
        printf("No se pudo obtener el valor de main_api del script\n");
    }*/

    // return 0;

    // Obtener la longitud de main_api
    // size_t main_api_len = strlen(getConfig().main_api);
    size_t main_api_len = strlen(main_api);
    const char *suffix = "/api/nfnode/access";
    size_t suffix_len = strlen(suffix);

    // Calcular la longitud total de la cadena resultante
    size_t total_len = main_api_len + suffix_len + 1; // +1 para el carácter nulo '\0'

    // Asignar memoria suficiente para la cadena concatenada
    char *concatenated_url = malloc(total_len);

    // Copiar main_api en la cadena concatenada
    strcpy(concatenated_url, main_api);

    // Concatenar el sufijo
    strcat(concatenated_url, suffix);

    // Usar concatenated en tu PostRequestOptions
    PostRequestOptions options = {
        .url = concatenated_url,
        .body = jsonDataString,
        .filePath = NULL,
        .key = NULL,
        .writeFunction = processAccessKeyResponse,
        .writeData = accessKey};

    int resultPost = performHttpPost(&options);
    json_object_put(jsonData);

    if (resultPost == 1)
    {
        printf("[access] Request was successful.\n");
        return 1;
    }
    else
    {
        printf("[access] Request failed.\n");
        return 0;
    }

    free(concatenated_url);
};

void configureWithAccessStatus(int accessStatus)
{
    printf("[access] Configuring with access status\n");
    if (accessStatus == 0)
    {
        printf("[access] Access status is 'initial'\n");
        state.setup = 1;
        state.accounting = 0;

        stopOpenNds();
    }
    else if (accessStatus == 1)
    {
        printf("[access] Access status is 'banned'\n");
        state.setup = 0;
        state.accounting = 0;

        stopOpenNds();
    }
    else if (accessStatus == 2)
    {
        printf("[access] Access status is 'setup-pending'\n");
        state.setup = 1;
        state.accounting = 0;

        stopOpenNds();
    }
    else if (accessStatus == 3)
    {
        printf("[access] Access status is 'setup-approved'\n");
        state.setup = 0;
        state.accounting = 1;

        completeSetup();
        startOpenNds();
    }
    else if (accessStatus == 4)
    {
        printf("[access] Access status is 'setup-completed'\n");
        state.setup = 0;
        state.accounting = 1;

        startOpenNds();
    }
}

void accessTask()
{
    printf("[access] Access task\n");

    int isExpired = checkAccessKeyNearExpiration(state.accessKey);
    if (isExpired == 1 || state.accessKey->key == NULL)
    {
        requestAccessKey(state.accessKey);
        writeAccessKey(state.accessKey);
        configureWithAccessStatus(state.accessStatus);
    }
    else
    {
        printf("[access] key is still valid\n");
    }
}