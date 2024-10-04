#ifndef RESULT_H
#define RESULT_H

#include <stdbool.h>

typedef struct {
    bool is_error;
    int error_code;
    void* value;
} Result;

#endif /* RESULT_H  */
