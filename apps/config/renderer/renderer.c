#include "renderer.h"
#include "core/console.h"
#include "core/script_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>  

static Console csl = {.topic = "renderer"};

// Paths for development mode
//#define DEV_CONFIG_FILE "./scripts/dev/wayru_config.json"
//#define DEV_RENDERER_SCRIPT "./scripts/dev/renderer_applier.uc"
#define DEV_CONFIG_FILE "/home/wayru/Firmware/wayru-os-services/apps/config/scripts/dev/wayru_config.json"
#define DEV_RENDERER_SCRIPT "/home/wayru/Firmware/wayru-os-services/apps/config/scripts/dev/renderer_applier.uc"

#define DEV_UCODE_PATH "/usr/local/bin/ucode"

// Paths for OpenWrt mode
#define OPENWRT_CONFIG_FILE "/tmp/wayru_config.json"
#define OPENWRT_RENDERER_SCRIPT "/etc/wayru-config/scripts/renderer_applier.uc"
#define OPENWRT_UCODE_PATH "/usr/bin/ucode"

static int write_config_file(const char *json_config, const char *filepath) {
    FILE *fp = fopen(filepath, "w");
    if (!fp) {
        console_error(&csl, "Failed to create config file: %s", filepath);
        return -1;
    }

    fprintf(fp, "%s", json_config);
    fclose(fp);
    return 0;
}

static void log_script_output(const char *output) {
    if (!output) return;
    
    // Crear una copia para procesar
    char *output_copy = strdup(output);
    if (!output_copy) return;
    
    char *line = strtok(output_copy, "\n");
    while (line != NULL) {
        // Trim whitespace
        while (*line == ' ' || *line == '\t') line++;
        
        if (strlen(line) > 0) {
            // Determinar el nivel de log basado en el contenido
            if (strstr(line, "Error") || strstr(line, "error") || strstr(line, "ERROR")) {
                console_error(&csl, "Script: %s", line);
            } else if (strstr(line, "Warning") || strstr(line, "warning") || strstr(line, "WARN")) {
                console_warn(&csl, "Script: %s", line);
            } else if (strstr(line, "#") && strlen(line) > 1) {
                // Comentarios del script
                console_debug(&csl, "Script: %s", line);
            } else {
                // Salida normal del script
                console_info(&csl, "Script: %s", line);
            }
        }
        line = strtok(NULL, "\n");
    }
    
    free(output_copy);
}

int apply_config(const char *json_config, bool dev_mode) {
    if (!json_config) {
        console_error(&csl, "Invalid JSON config");
        return -1;
    }

    const char *config_file = dev_mode ? DEV_CONFIG_FILE : OPENWRT_CONFIG_FILE;
    const char *renderer_script = dev_mode ? DEV_RENDERER_SCRIPT : OPENWRT_RENDERER_SCRIPT;
    const char *ucode_path = dev_mode ? DEV_UCODE_PATH : OPENWRT_UCODE_PATH;

    // Verify ucode exists
    if (access(ucode_path, X_OK) != 0) {
        console_error(&csl, "ucode not found at %s", ucode_path);
        return -1;
    }

    // Write JSON to config file
    if (write_config_file(json_config, config_file) != 0) {
        return -1;
    }

    // Prepare command to run renderer script
    char command[512];
    snprintf(command, sizeof(command), 
             "%s %s %s 2>&1",  // Redirigir stderr a stdout
             ucode_path, renderer_script, config_file);

    console_info(&csl, "Running renderer in %s mode", dev_mode ? "development" : "OpenWrt");
    console_debug(&csl, "Command: %s", command);

    // Execute renderer script y capturar código de salida
    FILE *fp = popen(command, "r");
    if (!fp) {
        console_error(&csl, "Failed to execute renderer script");
        if (!dev_mode) {
            unlink(config_file);
        }
        return -1;
    }

    // Leer toda la salida
    char *result = NULL;
    size_t result_size = 0;
    char buffer[1024];
    
    while (fgets(buffer, sizeof(buffer), fp)) {
        size_t buffer_len = strlen(buffer);
        result = realloc(result, result_size + buffer_len + 1);
        if (!result) {
            console_error(&csl, "Memory allocation failed");
            pclose(fp);
            return -1;
        }
        
        if (result_size == 0) {
            strcpy(result, buffer);
        } else {
            strcat(result, buffer);
        }
        result_size += buffer_len;
    }

    // Obtener código de salida
    int exit_code = pclose(fp);
    int script_exit_code = WEXITSTATUS(exit_code);

    if (result) {
        console_info(&csl, "Renderer script output:");
        log_script_output(result);
        free(result);
    }

    // Clean up temporary file only in OpenWrt mode
    if (!dev_mode) {
        unlink(config_file);
    }

    // Verificar si el script falló
    if (script_exit_code != 0) {
        console_error(&csl, "Renderer script failed with exit code %d", script_exit_code);
        return -1;
    }

    console_info(&csl, "Configuration rendering completed successfully");
    return 0;
}