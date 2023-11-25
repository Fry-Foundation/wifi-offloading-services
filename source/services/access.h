#ifndef ACCESS_H
#define ACCESS_H

#include <time.h>

typedef struct
{
    char *key;
    time_t createdAt;
    time_t expiresAt;
} AccessKey;

AccessKey* initAccessKey();

int readAccessKey(AccessKey *accessKey);

void writeAccessKey(AccessKey *accessKey);

int checkAccessKeyExpiration(AccessKey *accessKey);

int requestAccessKey(AccessKey *accessKey);

void accessTask();

#endif /* ACCESS_H */