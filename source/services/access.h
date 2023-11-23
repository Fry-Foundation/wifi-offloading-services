#ifndef ACCESS_H
#define ACCESS_H

typedef struct
{
    char *key;
    int createdAt;
    int expiresAt;
} AccessKey;

AccessKey readAccessKey();

void writeAccessKey();

void checkAccessKey();

void requestAccessKey();

#endif /* ACCESS_H */