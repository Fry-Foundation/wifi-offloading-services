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

    snprintf(key_path, sizeof(key_path), "%s/%s", config.data_path, KEY_FILE_NAME);
    snprintf(csr_path, sizeof(csr_path), "%s/%s", config.data_path, CSR_FILE_NAME);
    snprintf(cert_path, sizeof(cert_path), "%s/%s", config.data_path, CERT_FILE_NAME);

    // Print the paths for debugging
    console(CONSOLE_DEBUG, "Key path: %s", key_path);
    console(CONSOLE_DEBUG, "CSR path: %s", csr_path);
    console(CONSOLE_DEBUG, "Cert path: %s", cert_path);

    console(CONSOLE_INFO, "Running script to generate key, CSR and obtain signed certificate for mqtt");

    // printf(config.data_path);
    // char* cert = run_sign_cert(key_path, csr_path, cert_path);

    // Generate private key
    EVP_PKEY *pkey = generate_key_pair(Rsa);
    bool save_pkey_result = save_private_key_in_pem(pkey, key_path);
    console(CONSOLE_DEBUG, "Save private key result: %d", save_pkey_result);

    // Generate CSR
    generate_csr(pkey, csr_path);

    // Send CSR to backend to be signed
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
    // - Verify that the certificate is valid with the CA cert that we have

    // // Function to load a certificate from a PEM file
    // X509* load_certificate(const char *cert_path) {
    //     FILE *cert_file = fopen(cert_path, "rb");
    //     if (!cert_file) {
    //         fprintf(stderr, "Unable to open certificate file: %s\n", cert_path);
    //         return NULL;
    //     }
    //     X509 *cert = PEM_read_X509(cert_file, NULL, NULL, NULL);
    //     fclose(cert_file);
    //     return cert;
    // }

    // // Function to verify a certificate against a CA certificate
    // int verify_certificate(const char *cert_path, const char *ca_cert_path) {
    //     X509 *cert = load_certificate(cert_path);
    //     if (!cert) {
    //         fprintf(stderr, "Failed to load certificate: %s\n", cert_path);
    //         return 0;
    //     }

    //     X509 *ca_cert = load_certificate(ca_cert_path);
    //     if (!ca_cert) {
    //         fprintf(stderr, "Failed to load CA certificate: %s\n", ca_cert_path);
    //         X509_free(cert);
    //         return 0;
    //     }

    //     // Set up a certificate store and add the CA certificate to it
    //     X509_STORE *store = X509_STORE_new();
    //     if (!store) {
    //         fprintf(stderr, "Failed to create X509_STORE\n");
    //         X509_free(cert);
    //         X509_free(ca_cert);
    //         return 0;
    //     }
    //     X509_STORE_add_cert(store, ca_cert);

    //     // Create a context for the verification
    //     X509_STORE_CTX *ctx = X509_STORE_CTX_new();
    //     if (!ctx) {
    //         fprintf(stderr, "Failed to create X509_STORE_CTX\n");
    //         X509_STORE_free(store);
    //         X509_free(cert);
    //         X509_free(ca_cert);
    //         return 0;
    //     }

    //     X509_STORE_CTX_init(ctx, store, cert, NULL);

    //     // Verify the certificate
    //     int ret = X509_verify_cert(ctx);
    //     if (ret == 1) {
    //         printf("Certificate is valid.\n");
    //     } else {
    //         int err = X509_STORE_CTX_get_error(ctx);
    //         printf("Certificate verification failed: %s\n", X509_verify_cert_error_string(err));
    //     }

    //     // Cleanup
    //     X509_STORE_CTX_free(ctx);
    //     X509_STORE_free(store);
    //     X509_free(cert);
    //     X509_free(ca_cert);

    //     return ret == 1;
    // }


    // if (cert) {
    // console(CONSOLE_INFO, "Certificate signed and received");
    // } else {
    // console(CONSOLE_ERROR, "Error receiving signed certificate");
    // }
}