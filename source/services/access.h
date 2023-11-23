#ifndef ACCESS_H
#define ACCESS_H

#include <time.h>

typedef struct
{
    char *key;
    time_t createdAt;
    time_t expiresAt;
} AccessKey;

int readAccessKey(AccessKey *accessKey);

void writeAccessKey(AccessKey *accessKey);

int checkAccessKey();

int requestAccessKey(AccessKey *accessKey);

#endif /* ACCESS_H */