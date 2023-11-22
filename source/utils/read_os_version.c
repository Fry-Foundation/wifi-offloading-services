#include <stdio.h>
#include <string.h>
#include "read_os_version.h"
#include "../shared_store.h"

#define MAX_LINE_LENGTH 256

char* readOSVersion() {
    if (sharedStore.devMode == 1) {
        printf("Running in dev mode, returning 1.0.0\n");
        return "1.0.0";
    }

    FILE *file = fopen("/etc/openwrt_release", "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

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

    printf("DISTRIB_ID: %s\n", distrib_id);
    printf("DISTRIB_RELEASE: %s\n", distrib_release);

    fclose(file);
    return distrib_release;   
}