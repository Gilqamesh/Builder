#include "client.h"

#include <stdio.h>
#include <stdlib.h>

struct client {
    int _;
};

client_t client__create() {
    printf("client created\n");
    client_t result = calloc(1, sizeof(*result));
    return result;
}

void client__destroy(client_t self) {
    printf("client destroyed\n");
    free(self);
}

void client__run(client_t self) {
    (void) self;
    printf("Hello from client\n");
}
