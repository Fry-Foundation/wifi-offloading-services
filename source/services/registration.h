#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <stdbool.h>

typedef struct {
    char *wayru_device_id;
    char *access_key;
} Registration;

bool init_registration(char *mac, char *model, char *brand);
Registration get_registration();
void clean_registration(Registration *registration);

#endif /* REGISTRATION_H */
