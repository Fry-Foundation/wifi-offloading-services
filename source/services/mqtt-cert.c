#include "mqtt-cert.h"
#include "lib/console.h"
#include "lib/key_pair.h"
#include "lib/cert_audit.h"
#include "services/access_token.h"
#include "services/config.h"
#include <lib/http-requests.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "lib/retry.h"

#define KEY_FILE_NAME "device.key"
#define CSR_FILE_NAME "device.csr"
#define CERT_FILE_NAME "device.crt"
#define CA_CERT_FILE_NAME "ca.crt"
#define CA_ENDPOINT "/certificate-signing/ca"
#define BACKEND_ENDPOINT "/certificate-signing/sign"

static Console csl = {
    .topic = "mqtt certificates",
    .level = CONSOLE_DEBUG,
};

bool get_ca_cert(AccessToken *access_token) {
    char url[256];
    snprintf(url, sizeof(url), "%s%s", config.accounting_api, CA_ENDPOINT);
    print_debug(&csl, "Getting CA certificate from: %s", url);

    char ca_cert_path[256];
    snprintf(ca_cert_path, sizeof(ca_cert_path), "%s/%s", config.data_path, CA_CERT_FILE_NAME);

    HttpDownloadOptions get_ca_options = {
        .url = url,
        .bearer_token = access_token->token,
        .download_path = ca_cert_path,
    };

    HttpResult result = http_download(&get_ca_options);
    if (result.is_error) {
        print_error(&csl, "Failed to download CA certificate: %s", result.error);
        return false;
    }else{
        print_info(&csl, "CA certificate downloaded successfully");
    }

    // Verify that the downloaded CA certificate is valid
    int verify_result = validate_ca_cert(ca_cert_path);
    if (verify_result == 1) {
        return true;
    } else {
        return false;
    }
}

// char backend_url[256] = "https://wifi.api.internal.wayru.tech/certificate-signing/sign";

// char backend_url[256] = "https://wifi.api.dev.wayru.tech/certificate-signing/sign";

/*char *run_sign_cert(const char *key_path, const char *csr_path, const char *cert_path) {
    char script_file[256];
    snprintf(script_file, sizeof(script_file), "%s/sign_cert.sh", config.scripts_path);

    char command[1024];
    snprintf(command, sizeof(command), "%s %s %s %s %s", script_file, key_path, csr_path, cert_path, backend_url);

    char *result = run_script(command);
    return result;
}*/

bool attempt_ca_cert(AccessToken *access_token){
    RetryConfig config;
    config.retry_func = get_ca_cert;
    config.retry_params = access_token;
    config.attempts = 3;
    config.delay_seconds = 3;
    bool result = retry(&config);
    if (result == true) {
        print_info(&csl,"CA certificate is available");
        return true;
    } else {
        print_error(&csl, "No CA certificate after %d attempts ... exiting", config.attempts);
        return false;
    }
}

bool generate_and_sign_cert(AccessToken *access_token) {
    char key_path[256];
    char csr_path[256];
    char cert_path[256];
    char ca_cert_path[256];
    char backend_url[256];

    snprintf(key_path, sizeof(key_path), "%s/%s", config.data_path, KEY_FILE_NAME);
    snprintf(csr_path, sizeof(csr_path), "%s/%s", config.data_path, CSR_FILE_NAME);
    snprintf(cert_path, sizeof(cert_path), "%s/%s", config.data_path, CERT_FILE_NAME);
    snprintf(ca_cert_path, sizeof(ca_cert_path), "%s/%s", config.data_path, CA_CERT_FILE_NAME);
    snprintf(backend_url, sizeof(backend_url), "%s%s", config.accounting_api, BACKEND_ENDPOINT);

    // Print the paths for debugging
    print_debug(&csl, "Key path: %s", key_path);
    print_debug(&csl, "CSR path: %s", csr_path);
    print_debug(&csl, "Cert path: %s", cert_path);
    print_debug(&csl, "CA Cert path: %s", ca_cert_path);
    print_debug(&csl, "Backend URL: %s", backend_url);

    // Check if the certificate already exists and is valid
    print_debug(&csl, "Checking if certificate already exists and is valid (mqtt)...");
    int initial_verify_result = verify_certificate(cert_path, ca_cert_path);
    print_debug(&csl, "Checking if existing certificate matches key (mqtt)...");
    int initial_key_cert_match_result = validate_key_cert_match(key_path, cert_path);
    if (initial_verify_result == 1 && initial_key_cert_match_result == 1 ) {
        print_info(&csl, "Existing certificate is valid. No further action required (mqtt).");
        return true;
    } else {
        print_info(&csl, "Certificate does not exist or is not valid. Generating a new one (mqtt).");
    }

    // Generate private key
    print_debug(&csl, "Generating private key (mqtt)...");
    EVP_PKEY *pkey = generate_key_pair(Rsa);
    bool save_pkey_result = save_private_key_in_pem(pkey, key_path);
    print_info(&csl, "Save private key result: %d", save_pkey_result);

    // Generate CSR
    print_debug(&csl, "Generating CSR (mqtt)....");
    generate_csr(pkey, csr_path);
    print_debug(&csl, "Save csr result: %d", save_pkey_result);

    // Send CSR to backend to be signed
    print_debug(&csl, "Sending CSR to be signed (mqtt)....");
    HttpPostOptions post_cert_sign_options = {
        .url = backend_url,
        .upload_file_path = csr_path,
        .bearer_token = access_token->token,
    };

    HttpResult result = http_post(&post_cert_sign_options);
    if (result.is_error) {
        print_error(&csl, "Failed to sign certificate (mqtt): %s", result.error);
        return false;
    }

    if (result.response_buffer == NULL) {
        print_error(&csl, "Failed to sign certificate (mqtt): no response");
        return false;
    }

    // Save the signed certificate
    FILE *file = fopen(cert_path, "wb");
    if (file == NULL) {
        print_error(&csl, "Failed to open file for writing (mqtt): %s", cert_path);
        free(result.response_buffer);
        return false;
    }

    fwrite(result.response_buffer, 1, strlen(result.response_buffer), file);
    fclose(file);
    free(result.response_buffer);

    // Check that the written backend response is OK
    // Verify that the certificate is valid with the CA cert that we have
    print_debug(&csl, "Verifying signed certificate (mqtt)...");
    int verify_result = verify_certificate(cert_path, ca_cert_path);
    if (verify_result == 1) {
        print_info(&csl, "Certificate verification successful (mqtt).");
    } else {
        print_error(&csl, "Certificate verification failed (mqtt).");
        return false;
    }

    print_debug(&csl, "Verifying if new key matches certificate...");
    int key_cert_match_result = validate_key_cert_match(key_path, cert_path);
    if(key_cert_match_result == 1){
        print_info(&csl, "Key matches certificate");
        return true;
    }else{
        print_error(&csl, "Key does not match certificate");
        return false;
    }
}

bool attempt_generate_and_sign(AccessToken *access_token){
    RetryConfig config;
    config.retry_func = generate_and_sign_cert;
    config.retry_params = access_token;
    config.attempts = 3;
    config.delay_seconds = 3;
    bool result = retry(&config);
    if (result == true) {
        print_info(&csl, "Certificate generated and signed successfully");
        return true;
    } else {
        print_error(&csl, "Failed to generate and sign certificate after %d attempts ... exiting", config.attempts);
        return false;
    }
}
