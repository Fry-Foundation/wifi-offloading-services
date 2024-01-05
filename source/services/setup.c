#include <stdio.h>
#include "setup.h"
#include "../store/state.h"
#include "../utils/requests.h"
#include "../store/config.h"

// #define REQUEST_SETUP_ENDPOINT "https://api.wayru.tech/api/nfNode/setup"
// #define COMPLETE_SETUP_ENDPOINT "https://api.wayru.tech/api/nfNode/setup/complete"

// Backend should handle setup requests that have already been created for this access key
// If no setup request exists, create one
void requestSetup()
{
    printf("[setup] Request setup\n");
    printf("[setup] Access key: %s\n", state.accessKey->key);

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

    // Obtener la longitud de main_api
    // size_t main_api_len = strlen(getConfig().main_api);
    size_t main_api_len = strlen(main_api);
    const char *suffix = "/api/nfNode/setup";
    size_t suffix_len = strlen(suffix);

    // Calcular la longitud total de la cadena resultante
    size_t total_len = main_api_len + suffix_len + 1; // +1 para el carácter nulo '\0'

    // Asignar memoria suficiente para la cadena concatenada
    char *concatenated_url = malloc(total_len);

    // Copiar main_api en la cadena concatenada
    strcpy(concatenated_url, main_api);

    // Concatenar el sufijo
    strcat(concatenated_url, suffix);

    PostRequestOptions requestSetup = {
        .url = concatenated_url,
        .key = state.accessKey->key,
        .body = NULL,
        .filePath = NULL,
        .writeFunction = NULL,
        .writeData = NULL,
    };

    int result = performHttpPost(&requestSetup);
    if (result == 1)
    {
        printf("[setup] setup request was a success\n");
    }
    else
    {
        printf("[setup] setup request failed\n");
    }
    free(concatenated_url);
}

// @TODO: Pending backend implementation
int checkApprovedSetup()
{
    printf("[setup] Not yet implemented - Check if the setup has been approved\n");
    printf("[setup] Not yet implemented - Access key: %s\n", state.accessKey->key);
}

void completeSetup()
{
    printf("[setup] Complete setup\n");
    printf("[setup] Access key: %s\n", state.accessKey->key);

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

    // Obtener la longitud de main_api
    size_t main_api_len = strlen(main_api);
    const char *suffix = "/api/nfNode/setup/complete";
    size_t suffix_len = strlen(suffix);

    // Calcular la longitud total de la cadena resultante
    size_t total_len = main_api_len + suffix_len + 1; // +1 para el carácter nulo '\0'

    // Asignar memoria suficiente para la cadena concatenada
    char *concatenated_url = malloc(total_len);

    // Copiar main_api en la cadena concatenada
    strcpy(concatenated_url, main_api);

    // Concatenar el sufijo
    strcat(concatenated_url, suffix);

    PostRequestOptions completeSetupOptions = {
        .url = concatenated_url,
        .key = state.accessKey->key,
        .body = NULL,
        .filePath = NULL,
        .writeFunction = NULL,
        .writeData = NULL,
    };

    performHttpPost(&completeSetupOptions);

    free(concatenated_url);
}

void setupTask()
{
    if (state.setup != 1)
    {
        printf("[setup] Setup is disabled\n");
        return;
    }

    printf("[setup] Setup task\n");

    if (state.accessStatus == 0)
    {
        requestSetup();
    }
    else if (state.accessStatus == 2)
    {
        checkApprovedSetup();
    }
    else if (state.accessStatus == 3)
    {
        // Note: We currently complete the setup from the access task
        // since that call receives the updated status value first
        // @TODO: Implement a status endpoint that we can call from here
        // completeSetup();
    }
}