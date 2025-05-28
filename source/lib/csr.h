#ifndef CSR_H
#define CSR_H

#include <lib/result.h>
#include <openssl/evp.h>
#include <stdbool.h>

typedef struct {
    const unsigned char *country;
    const unsigned char *state;
    const unsigned char *locality;
    const unsigned char *organization;
    const unsigned char *organizational_unit;
    const unsigned char *common_name;
} CSRInfo;

Result generate_csr(EVP_PKEY *pkey, const char *csr_filepath, CSRInfo *info);

#endif /* CSR_H */
