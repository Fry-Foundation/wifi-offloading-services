#include <stdio.h>
#include <string.h>
#include "read_os_version.h"
#include "../shared_store.h"

#define MAX_LINE_LENGTH 256

char* readOSVersion() {
    if (sharedStore.devMode == 1) {
        printf("Running in dev mode, returning os version 1.0.0\n");
        return strdup("1.0.0");
    }

    FILE *file = fopen("/etc/openwrt_release", "r");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    char *osVersion = NULL;

    char line[MAX_LINE_LENGTH];
    char distrib_id[MAX_LINE_LENGTH];
    char distrib_release[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "DISTRIB_ID", 10) == 0) {
            sscanf(line, "DISTRIB_ID='%[^']'", distrib_id);
        } else if (strncmp(line, "DISTRIB_RELEASE", 15) == 0) {
            sscanf(line, "DISTRIB_RELEASE='%[^']'", distrib_release);
        }
    }

    if (strchr(distrib_release, '\n') != NULL) {
        distrib_release[strcspn(distrib_release, "\n")] = 0;
    }

    fclose(file);    

    // Allocate memory and copy the version string
    if (*distrib_release != '\0') { // Check if distrib_release is not empty
        osVersion = strdup(distrib_release);
        if (osVersion == NULL) {
            perror("Memory allocation failed for osVersion");
            fclose(file);
            return NULL;
        }
    } else {
        fprintf(stderr, "OS version string is empty\n");
    }

    return osVersion;
}