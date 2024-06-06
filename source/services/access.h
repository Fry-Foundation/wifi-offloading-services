#ifndef ACCESS_H
#define ACCESS_H

#include <time.h>

typedef struct {
    char *key;
    time_t created_at;
    time_t expires_at;
} AccessKey;

extern AccessKey access_key;

void init_access_service();

void clean_access_service();

#endif /* ACCESS_H */
