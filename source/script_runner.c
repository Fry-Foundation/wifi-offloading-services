#include "script_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void runScriptAndSaveOutput(const char *scriptPath, const char *outputPath) {
    char buffer[1024];  // Buffer for reading script output
    FILE *scriptPipe, *outputFile;

    // Open the script for execution
    scriptPipe = popen(scriptPath, "r");
    if (scriptPipe == NULL) {
        perror("Failed to run script");
        return;
    }

    // Open the output file for writing
    outputFile = fopen(outputPath, "w");
    if (outputFile == NULL) {
        perror("Failed to open output file for writing");
        pclose(scriptPipe);
        return;
    }

    // Read the output of the script and write it to the output file
    while (fgets(buffer, sizeof(buffer), scriptPipe) != NULL) {
        fputs(buffer, outputFile);
    }

    // Close the script pipe and the output file
    pclose(scriptPipe);
    fclose(outputFile);

    printf("Script executed successfully, output saved to: %s\n", outputPath);
}
