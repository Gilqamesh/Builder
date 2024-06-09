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

size_t msg_queue__read(msg_queue_t self, void* buffer, size_t buffer_size); // returns n of bytes that would have been read
size_t msg_queue__read_str(msg_queue_t self, char* buffer, size_t buffer_size); // null-terminates message
size_t msg_queue__write(msg_queue_t self, void* msg, size_t msg_size); // returns n of bytes sent
size_t msg_queue__write_str(msg_queue_t self, const char* format, ...); // returns n of bytes sent
size_t msg_queue__vwrite_str(msg_queue_t self, const char* format, va_list ap); // returns n of bytes sent

void msg_queue__print(msg_queue_t self);

#endif // MSG_H
