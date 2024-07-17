#include "mqtt-cert.h"
#include "lib/console.h"
#include "lib/key_pair.h"
#include "lib/script_runner.h"
#include "services/access.h"
#include "services/config.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <lib/requests.h>

#define KEY_FILE_NAME "device.key"
#define CSR_FILE_NAME "device.csr"
#define CERT_FILE_NAME "device.crt"

char backend_url[256] = "https://wifi.api.internal.wayru.tech/certificate-signing/sign";

// char backend_url[256] = "https://wifi.api.dev.wayru.tech/certificate-signing/sign";

char *run_sign_cert(const char *key_path, const char *csr_path, const char *cert_path) {
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
    char ca_cert_path[256];

    if (config.dev_env == 1) {
        strcpy(ca_cert_path, "./source/certificates/ca.crt");
    } else {
        strcpy(ca_cert_path, "/etc/wayru-os-services/data/ca.crt");
    }

    snprintf(key_path, sizeof(key_path), "%s/%s", config.data_path, KEY_FILE_NAME);
    snprintf(csr_path, sizeof(csr_path), "%s/%s", config.data_path, CSR_FILE_NAME);
    snprintf(cert_path, sizeof(cert_path), "%s/%s", config.data_path, CERT_FILE_NAME);
    //snprintf(ca_cert_path, sizeof(ca_cert_path), "%s/%s", config.data_path, CA_CERT_FILE_NAME);

    // Print the paths for debugging
    console(CONSOLE_DEBUG, "Key path: %s", key_path);
    console(CONSOLE_DEBUG, "CSR path: %s", csr_path);
    console(CONSOLE_DEBUG, "Cert path: %s", cert_path);
    console(CONSOLE_DEBUG, "CA Cert path: %s", ca_cert_path);

    // char* cert = run_sign_cert(key_path, csr_path, cert_path);

    // Generate private key
    console(CONSOLE_DEBUG, "Generating private key (mqtt)...");
    EVP_PKEY *pkey = generate_key_pair(Rsa);
    bool save_pkey_result = save_private_key_in_pem(pkey, key_path);
    console(CONSOLE_INFO, "Save private key result: %d", save_pkey_result);

    // Generate CSR
    console(CONSOLE_DEBUG, "Generating CSR (mqtt)....");
    generate_csr(pkey, csr_path);
    console(CONSOLE_DEBUG, "Save csr result: %d", save_pkey_result);

    // Send CSR to backend to be signed
    console(CONSOLE_DEBUG, "Sending CSR to be signed (mqtt)....");
    PostRequestOptions post_cert_sign_options = {
        .url = backend_url,
        .key = access_key.public_key,
        .body = NULL,
        .filePath = csr_path,
        .resultFilePath = cert_path,
        .writeFunction = NULL,
        .writeData = NULL,
    };

    performHttpPost(&post_cert_sign_options);

    // Check backend response
    // Verify that the certificate is valid with the CA cert that we have
    console(CONSOLE_DEBUG, "Verifying signed certificate (mqtt)...");
    int verify_result = verify_certificate(cert_path, ca_cert_path);

    if (verify_result == 1) {
        console(CONSOLE_INFO, "Certificate verification successful (mqtt).");
    } else{
        console(CONSOLE_ERROR, "Certificate verification failed (mqtt).");
    }
    
    
}