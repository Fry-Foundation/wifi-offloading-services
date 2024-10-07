#include <stdio.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include "lib/console.h"

int validate_ca_cert(const char *ca_cert_path) {
    FILE *ca_file = fopen(ca_cert_path, "r");
    if (!ca_file) {
        console(CONSOLE_ERROR, "Error opening CA certificate: %s", ca_cert_path);
        return 0;
    }

    X509 *ca_cert = PEM_read_X509(ca_file, NULL, NULL, NULL);
    fclose(ca_file);
    if (!ca_cert) {
        console(CONSOLE_ERROR, "Error reading CA certificate: %s", ca_cert_path);
        return 0;
    }

    int is_ca = X509_check_ca(ca_cert);
    if (is_ca) {
        console(CONSOLE_INFO, "The certificate %s is valid and is a CA.", ca_cert_path);
    } else {
        console(CONSOLE_ERROR, "The certificate %s is not a CA.", ca_cert_path);
    }

    X509_free(ca_cert);
    return is_ca;
}

int validate_key_cert_match(const char *keyFile, const char *certFile) {
    FILE *key_fp = fopen(keyFile, "r");
    if (!key_fp) {
        console(CONSOLE_ERROR, "Error could not open the .key file");
        return -1;
    }

    EVP_PKEY *pkey = PEM_read_PrivateKey(key_fp, NULL, NULL, NULL);
    fclose(key_fp);

    if (!pkey) {
        console(CONSOLE_ERROR, "Error reading the private key.");
        ERR_print_errors_fp(stderr);
        return -1;
    }

    FILE *cert_fp = fopen(certFile, "r");
    if (!cert_fp) {
        console(CONSOLE_ERROR, "Error could not open the .crt file");
        EVP_PKEY_free(pkey);
        return -1;
    }

    X509 *cert = PEM_read_X509(cert_fp, NULL, NULL, NULL);
    fclose(cert_fp);

    if (!cert) {
        console(CONSOLE_ERROR, "Error reading the certificate.");
        EVP_PKEY_free(pkey);
        ERR_print_errors_fp(stderr);
        return -1;
    }

    EVP_PKEY *pubkey = X509_get_pubkey(cert);
    if (!pubkey) {
        console(CONSOLE_ERROR, "Error getting the public key from the certificate.");
        EVP_PKEY_free(pkey);
        X509_free(cert);
        return -1;
    }

    int result = EVP_PKEY_cmp(pkey, pubkey);
    if (result == 1) {
        result = 1;  // Match
    } else if (result == 0) {
        result = 0;  // No match
    } else {
        result = -1;  // Error comparing
    }

    EVP_PKEY_free(pkey);
    EVP_PKEY_free(pubkey);
    X509_free(cert);

    return result;
}