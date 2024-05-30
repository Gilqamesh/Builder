#ifndef CLIENT_H
# define CLIENT_H

typedef struct client* client_t;

client_t client__create();
void client__destroy(client_t self);

void client__run(client_t self);

#endif // CLIENT_H
