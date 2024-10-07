#ifndef CERT_AUDIT_H
#define CERT_AUDIT_H

int validate_ca_cert(const char *ca_cert_path);
int validate_key_cert_match(const char *keyFile, const char *certFile);

#endif // CERT_AUDIT_H