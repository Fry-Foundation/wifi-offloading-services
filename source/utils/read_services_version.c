#include <stdio.h>
#include <string.h>
#include "read_services_version.h"
#include "../shared_store.h"

#define MAX_VERSION_LENGTH 256

char* readServicesVersion() {
    if (sharedStore.devMode == 1) {
        printf("Running in dev mode, returning services version 1.0.0\n");
        return "1.0.0";
    }

    FILE *file = fopen("VERSION", "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    char version[MAX_VERSION_LENGTH];

    fgets(version, MAX_VERSION_LENGTH, file);

    if (strchr(version, '\n') != NULL) {
        version[strcspn(version, "\n")] = 0;
    }

    fclose(file);
    return version;   
}