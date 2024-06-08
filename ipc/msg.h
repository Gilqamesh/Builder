#ifndef MSG_H
# define MSG_H

# include <stddef.h>
# include <stdarg.h>
# include <stdint.h>

# include "sem.h"

typedef struct msg_queue {
    int id;
    struct sem sem;
} *msg_queue_t;

int msg_queue__create(msg_queue_t self, uint8_t nonnull_key_id);
void msg_queue__destroy(msg_queue_t self);

/**
 * Not thread-safe
*/
size_t msg_queue__read(msg_queue_t self, char* buffer, size_t buffer_size);
/**
 * Not thread-safe
*/
void msg_queue__write(msg_queue_t self, const char* format, ...);
void msg_queue__vwrite(msg_queue_t self, const char* format, va_list ap);

void msg_queue__print(msg_queue_t self);

#endif // MSG_H
