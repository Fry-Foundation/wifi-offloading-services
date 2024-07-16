#include "mqtt-cert.h"
#include "services/config.h"
#include "services/access.h"
#include "lib/console.h"
#include "lib/script_runner.h"
#include <string.h>
#include <stdio.h>

#define KEY_FILE_NAME "device.key"
#define CSR_FILE_NAME "device.csr"
#define CERT_FILE_NAME "device.crt"

char backend_url[256] = "https://wifi.api.internal.wayru.tech/certificate-signing/sign";

//char backend_url[256] = "https://wifi.api.dev.wayru.tech/certificate-signing/sign";

char* run_sign_cert(const char* key_path, const char* csr_path, const char* cert_path) {
    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s/sign_cert.sh", config.scripts_path);

    char command[1024];
    snprintf(command, sizeof(command), "%s %s %s %s %s", script_file, key_path, csr_path, cert_path, backend_url);

    char *result = run_script(command);
    return result;

}

void generate_and_sign_cert() {
    char key_path[256];
    char csr_path[256];
    char cert_path[256];

    snprintf(key_path, sizeof(key_path), "%s/%s", config.data_path, KEY_FILE_NAME);
    snprintf(csr_path, sizeof(csr_path), "%s/%s", config.data_path, CSR_FILE_NAME);
    snprintf(cert_path, sizeof(cert_path), "%s/%s", config.data_path, CERT_FILE_NAME);

    // Print the paths for debugging
    console(CONSOLE_DEBUG, "Key path: %s", key_path);
    console(CONSOLE_DEBUG, "CSR path: %s", csr_path);
    console(CONSOLE_DEBUG, "Cert path: %s", cert_path);

    console(CONSOLE_INFO, "Running script to generate key, CSR and obtain signed certificate for mqtt");
    
    // printf(config.data_path);
    char* cert = run_sign_cert(key_path, csr_path, cert_path);

    if (cert) {
        console(CONSOLE_INFO, "Certificate signed and received");
    } else {
        console(CONSOLE_ERROR, "Error receiving signed certificate");
    }
}