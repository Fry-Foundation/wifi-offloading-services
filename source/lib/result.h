#ifndef RESULT_H
#define RESULT_H

#include <stdbool.h>

#define ERROR_MESSAGE_LENGTH 256

typedef struct {
    int code;
    char message[ERROR_MESSAGE_LENGTH];
} Error;

typedef struct {
    bool ok;
    union {
        Error error;
        void* data;
    };
} Result;

Result ok(void* value);
Result error(int code, const char* message);

#endif /* RESULT_H  */
