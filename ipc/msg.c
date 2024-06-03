#include "msg.h"

#include <stdlib.h>
#include <errno.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

msg_queue_t msg_queue__create() {
    char buffer[256];

    ssize_t bytes_read = readlink("/proc/self/exe", buffer, sizeof(buffer));
    if (bytes_read < 0) {
        perror("readlink");
        return 0;
    }
    if (sizeof(buffer) <= (size_t) bytes_read) {
        // truncation
        return 0;
    }
    buffer[bytes_read] = '\0';
    key_t key = ftok(buffer, 1);
    if (key < 0) {
        perror("ftok");
        return 0;
    }

    int msg_queue = msgget(key, 0666 | IPC_CREAT);
    if (msg_queue < 0) {
        perror("msgget");
        return 0;
    }

    msg_queue_t result = calloc(1, sizeof(*result));
    result->id = msg_queue;

    return result;
}

void msg_queue__destroy(msg_queue_t self) {
    if (0 < self->id && msgctl(self->id, IPC_RMID, 0) < 0) {
        perror("msgctl");
    }
    free(self);
}

void msg_queue__destroy_from_child(msg_queue_t self) {
    free(self);
}

struct msg_buffer {
    long type;
    char buffer[4096];
};

size_t msg_queue__read(msg_queue_t self, char* buffer, size_t buffer_size) {
    size_t bytes_read = 0;
    
    static struct msg_buffer msg_buffer = {
        .type = 0
    };
    ssize_t msgrcv_result = msgrcv(self->id, &msg_buffer, sizeof(msg_buffer.buffer), msg_buffer.type, MSG_NOERROR | IPC_NOWAIT);
    if (msgrcv_result < 0) {
        if (errno != ENOMSG) {
            perror("msgrcv");
        }
    } else {
        bytes_read = buffer_size - 1 < (size_t) msgrcv_result ? buffer_size - 1 : (size_t) msgrcv_result;
        memcpy(buffer, msg_buffer.buffer, bytes_read);
        buffer[bytes_read] = '\0';
    }

    return bytes_read;
}

void msg_queue__write(msg_queue_t self, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    msg_queue__vwrite(self, format, ap);
    va_end(ap);
}

void msg_queue__vwrite(msg_queue_t self, const char* format, va_list ap) {
    static struct msg_buffer msg_buffer = {
        .type = 1
    };
    int bytes_written = vsnprintf(msg_buffer.buffer, sizeof(msg_buffer.buffer), format, ap);
    if (bytes_written < 0) {
        return ;
    }

    if (msgsnd(self->id, &msg_buffer, sizeof(msg_buffer.buffer), 0) != 0) {
        perror("msgsnd");
    }
}

void msg_queue__print(msg_queue_t self) {
    (void) self;
}
