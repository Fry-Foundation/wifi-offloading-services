#ifndef PEAQ_ID_H
#define PEAQ_ID_H

#include <stdbool.h>

bool generate_key_pair(char *public_key_filename, char *private_key_filename);

char *read_private_key();

char *read_public_key();

void peaq_id_task();

#endif /* PEAQ_ID_H  */
