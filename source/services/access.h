#ifndef ACCESS_H
#define ACCESS_H

#include <time.h>

typedef struct
{
    char *key;
    time_t created_at;
    time_t expires_at;
} AccessKey;

AccessKey *init_access_key();

int read_access_key(AccessKey *access_key);

void write_access_key(AccessKey *access_key);

int request_access_key(AccessKey *access_key);

int check_access_key_near_expiration(AccessKey *access_key);

void configure_with_access_status(int access_status);

void access_task();

#endif /* ACCESS_H */