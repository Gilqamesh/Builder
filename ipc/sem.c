#include "sem.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

int sem__create(sem_t self, uint8_t nonnull_key_id) {
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

    self->ref_count = 0;
    self->id = semget(key, 1, IPC_CREAT | 0666);
    if (self->id < 0) {
        perror("semget");
        sem__destroy(self);
        return 1;
    }

    return 0;
}

void sem__destroy(sem_t self) {
    if (0 < self->id && semctl(self->id, 0, IPC_RMID) < 0) {
        perror("semctl IPC_RMID");
    }
}

void sem__reset_ref_counter(sem_t self) {
    self->ref_count = 0;
}

void sem__lock(sem_t self) {
    if (self->ref_count == 0) {
        struct sembuf sb[2] = {
            [0] = {
                .sem_flg = 0,
                .sem_num = 0,
                .sem_op  = 0
            },
            [1] = {
                .sem_flg = SEM_UNDO,
                .sem_num = 0,
                .sem_op  = 1
            }
        };
        while (semop(self->id, sb, 2)) {
            if (errno == EINTR) {
                continue ;
            }
            perror("semop lock");
            exit(errno);
        }
    }
    ++self->ref_count;
}

void sem__unlock(sem_t self) {
    if (self->ref_count == 1) {
        struct sembuf sb = {
            .sem_flg = 0,
            .sem_num = 0,
            .sem_op  = -1
        };
        while (semop(self->id, &sb, 1)) {
            if (1000 < sem__val(self)) {
                int dbg = 0;
                ++dbg;
            }
            if (errno == EINTR) {
                continue ;
            }
            perror("semop dec");
            exit(errno);
        }
    }
    assert(0 < self->ref_count);
    --self->ref_count;
}

int sem__get(sem_t self) {
    int semval = semctl(self->id, 0, GETVAL);
    if (semval == -1) {
        perror("semctl GETVAL");
        return -1;
    }
    return semval;
}

int sem__val(sem_t self) {
    int val = semctl(self->id, 0, GETVAL);
    if (val == -1) {
        perror("semctl GETVAL");
        return -1;
    }
    return val;
}

int sem__zcnt(sem_t self) {
    int val = semctl(self->id, 0, GETZCNT);
    if (val == -1) {
        perror("semctl GETZCNT");
        return -1;
    }
    return val;
}

int sem__ncnt(sem_t self) {
    int val = semctl(self->id, 0, GETNCNT);
    if (val == -1) {
        perror("semctl GETNCNT");
        return -1;
    }
    return val;
}

int sem__pid(sem_t self) {
    int val = semctl(self->id, 0, GETPID);
    if (val == -1) {
        perror("semctl GETPID");
        return -1;
    }
    return val;
}


void sem__print(sem_t self) {
    int semval = sem__val(self);
    if (semval != -1) {
        printf("Semaphore value (semval): %d\n", semval);
    }
    int semzcnt = sem__zcnt(self);
    if (semzcnt != -1) {
        printf("Number of processes waiting for zero (semzcnt): %d\n", semzcnt);
    }

    int semncnt = sem__ncnt(self);
    if (semncnt != -1) {
        printf("Number of processes waiting for increment (semncnt): %d\n", semncnt);
    }

    int sempid = sem__pid(self);
    if (sempid != -1) {
        printf("PID of last process that modified semaphore (sempid): %d\n", sempid);
    }
}
