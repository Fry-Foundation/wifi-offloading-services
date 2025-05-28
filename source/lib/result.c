#include "result.h"
#include <string.h>

Result ok(void *data) {
    Result result;
    result.ok = true;
    result.data = data;
    return result;
}

Result error(int code, const char *message) {
    Result result;
    result.ok = false;
    result.error.code = code;

    // Copy the error message into the fixed-size buffer, ensuring no overflow
    strncpy(result.error.message, message, ERROR_MESSAGE_LENGTH - 1);
    result.error.message[ERROR_MESSAGE_LENGTH - 1] = '\0';

    return result;
}
