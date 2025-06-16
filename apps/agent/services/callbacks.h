#ifndef CALLBACKS_H
#define CALLBACKS_H

// Access token refresh callback function type
typedef void (*AccessTokenRefreshCallback)(const char *new_token, void *context);

typedef struct {
    AccessTokenRefreshCallback on_token_refresh;
    void *context;
} AccessTokenCallbacks;

#endif /* CALLBACKS_H */