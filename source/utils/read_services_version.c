#include <stdio.h>
#include <string.h>
#include "read_services_version.h"
#include "../shared_store.h"

#define MAX_VERSION_LENGTH 256

int devMode = 0;

char *readServicesVersion()
{
    if (devMode == 1)
    {
        printf("Running in dev mode, returning services version 1.0.0\n");
        return strdup("1.0.0");
    }

    FILE *file = fopen("/etc/wayru/VERSION", "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return NULL;
    }

    char *servicesVersion = NULL;
    char version[MAX_VERSION_LENGTH];

    if (fgets(version, MAX_VERSION_LENGTH, file) == NULL)
    {
        fprintf(stderr, "Failed to read version\n");
        fclose(file);
        return NULL; // Handle failed read attempt
    }

    printf("Services Version is: %s\n", version);

    if (strchr(version, '\n') != NULL)
    {
        version[strcspn(version, "\n")] = 0;
    }

    fclose(file);

    // Allocate memory for the version string and return
    servicesVersion = strdup(version);
    if (servicesVersion == NULL)
    {
        perror("Memory allocation failed for dynamicVersion");
        return NULL;
    }

    printf("Dynamic version is: %s\n", servicesVersion);

    return servicesVersion;
}