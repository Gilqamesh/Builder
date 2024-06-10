#include "msg.h"

#include <stdlib.h>
#include <errno.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

int msg_queue__create(msg_queue_t self, uint8_t nonnull_key_id) {
    char buffer[256];

    ssize_t bytes_read = readlink("/proc/self/exe", buffer, sizeof(buffer));
    if (bytes_read < 0) {
        perror("readlink");
        return 1;
    }
    if (sizeof(buffer) <= (size_t) bytes_read) {
        // truncation
        return 1;
    }
    buffer[bytes_read] = '\0';
    key_t key = ftok(buffer, nonnull_key_id);
    if (key < 0) {
        perror("ftok");
        return 1;
    }

    self->id = msgget(key, 0666 | IPC_CREAT);
    if (self->id < 0) {
        perror("msgget");
        return 1;
    }

    if (sem__create(&self->sem, nonnull_key_id)) {
        msg_queue__destroy(self);
        return 1;
    }

    return 0;
}

void msg_queue__destroy(msg_queue_t self) {
    if (0 < self->id && msgctl(self->id, IPC_RMID, 0) < 0) {
        perror("msgctl");
    }

    sem__destroy(&self->sem);
}

struct msg_buffer {
    long type;
    char buffer[4096];
};

static struct msg_buffer g_msg_buffer; // as msg queue operations are atomic, this is fine

size_t msg_queue__read(msg_queue_t self, void* buffer, size_t buffer_size) {
    sem__lock(&self->sem);

    size_t bytes_read = 0;

    g_msg_buffer.type = 0;
    ssize_t msgrcv_result = msgrcv(self->id, &g_msg_buffer, sizeof(g_msg_buffer.buffer), g_msg_buffer.type, MSG_NOERROR | IPC_NOWAIT);

    if (msgrcv_result < 0) {
        if (errno != ENOMSG) {
            perror("msgrcv");
        }
    } else {
        bytes_read = msgrcv_result;
        size_t bytes_to_cpy = buffer_size < (size_t) msgrcv_result ? buffer_size: (size_t) msgrcv_result;
        memcpy(buffer, g_msg_buffer.buffer, bytes_to_cpy);
    }

    sem__unlock(&self->sem);

    return bytes_read;
}

size_t msg_queue__read_str(msg_queue_t self, char* buffer, size_t buffer_size) {
    sem__lock(&self->sem);

    size_t bytes_read = 0;

    g_msg_buffer.type = 0;
    ssize_t msgrcv_result = msgrcv(self->id, &g_msg_buffer, sizeof(g_msg_buffer.buffer), g_msg_buffer.type, MSG_NOERROR | IPC_NOWAIT);

    if (msgrcv_result < 0) {
        if (errno != ENOMSG) {
            perror("msgrcv");
        }
    } else if (0 < buffer_size) {
        bytes_read = msgrcv_result;
        size_t bytes_to_cpy = buffer_size - 1 < (size_t) msgrcv_result ? buffer_size - 1 : (size_t) msgrcv_result;
        memcpy(buffer, g_msg_buffer.buffer, bytes_to_cpy);
        buffer[bytes_to_cpy] = '\0';
    }

    sem__unlock(&self->sem);

    return bytes_read;

}

size_t msg_queue__write(msg_queue_t self, void* msg, size_t msg_size) {
    size_t result = msg_size;

    sem__lock(&self->sem);

    g_msg_buffer.type = 1;

    result = sizeof(g_msg_buffer.buffer) < msg_size ? sizeof(g_msg_buffer.buffer) : msg_size;
    memcpy(g_msg_buffer.buffer, msg, result);

    if (msgsnd(self->id, &g_msg_buffer, result, 0) != 0) {
        perror("msgsnd");
        result = 0;
    }

    sem__unlock(&self->sem);

    return result;
}

size_t msg_queue__write_str(msg_queue_t self, const char* format, ...) {
    sem__lock(&self->sem);

    va_list ap;
    va_start(ap, format);
    size_t result = msg_queue__vwrite_str(self, format, ap);
    va_end(ap);

    sem__unlock(&self->sem);

    return result;
}


size_t msg_queue__vwrite_str(msg_queue_t self, const char* format, va_list ap) {
    size_t result = 0;

    sem__lock(&self->sem);

    g_msg_buffer.type = 1;
    int bytes_written = vsnprintf(g_msg_buffer.buffer, sizeof(g_msg_buffer.buffer), format, ap);
    if (bytes_written < 0) {
        sem__unlock(&self->sem);
        return result;
    }

    result = sizeof(g_msg_buffer.buffer) <= (size_t) bytes_written ? sizeof(g_msg_buffer.buffer) : (size_t) bytes_written + 1;

    if (msgsnd(self->id, &g_msg_buffer, result, 0) != 0) {
        perror("msgsnd");
        result = 0;
    }

    sem__unlock(&self->sem);

    return result;
}

void msg_queue__print(msg_queue_t self) {
    sem__lock(&self->sem);
    struct msqid_ds msg_info;
    if (msgctl(self->id, IPC_STAT, &msg_info) == -1) {
        perror("msgctl IPC_STAT");
        sem__unlock(&self->sem);
        return ;
    }

    struct tm* st = localtime(&msg_info.msg_stime);
    struct tm* rt = localtime(&msg_info.msg_rtime);
    struct tm* ct = localtime(&msg_info.msg_ctime);

    printf(
        "Message queue info:\n"
        "  id: %d\n"
        "  permissions:\n"
        "    key:                      %d\n"
        "    effective UID of owner:   %d\n"
        "    effective GID of owner:   %d\n"
        "    effective UID of creator: %d\n"
        "    effective GID of creator: %d\n"
        "    mode:\n"
        "      read  by user:   %d\n"
        "      write by user:   %d\n"
        "      read  by group:  %d\n"
        "      write by group:  %d\n"
        "      read  by others: %d\n"
        "      write by others: %d\n"
        "    sequence number: %d\n"
        "  time of last send:    %02d/%02d/%d %02d:%02d:%02d\n"
        "  time of last receive: %02d/%02d/%d %02d:%02d:%02d\n"
        "  time of creation:     %02d/%02d/%d %02d:%02d:%02d\n"
        "  bytes in queue:              %lu\n"
        "  max bytes in queue:          %lu\n"
        "  number of messages in queue: %lu\n"
        "  pid of last sent:            %d\n"
        "  pid of last received:        %d\n",
        self->id,
        msg_info.msg_perm.__key,
        msg_info.msg_perm.uid,
        msg_info.msg_perm.gid,
        msg_info.msg_perm.cuid,
        msg_info.msg_perm.cgid,
        (msg_info.msg_perm.mode & 0400) != 0,
        (msg_info.msg_perm.mode & 0200) != 0,
        (msg_info.msg_perm.mode & 0040) != 0,
        (msg_info.msg_perm.mode & 0020) != 0,
        (msg_info.msg_perm.mode & 0004) != 0,
        (msg_info.msg_perm.mode & 0002) != 0,
        msg_info.msg_perm.__seq,
        st->tm_mday, st->tm_mon + 1, st->tm_year + 1900, st->tm_hour, st->tm_min, st->tm_sec,
        rt->tm_mday, rt->tm_mon + 1, rt->tm_year + 1900, rt->tm_hour, rt->tm_min, rt->tm_sec,
        ct->tm_mday, ct->tm_mon + 1, ct->tm_year + 1900, ct->tm_hour, ct->tm_min, ct->tm_sec,
        msg_info.__msg_cbytes,
        msg_info.msg_qbytes,
        msg_info.msg_qnum,
        msg_info.msg_lspid,
        msg_info.msg_lrpid
    );

    sem__unlock(&self->sem);
}
