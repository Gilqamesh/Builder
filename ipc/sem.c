#include "sem.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <unistd.h>

sem_t sem__create() {
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
    
    sem_t result = calloc(1, sizeof(*result));
    result->id = semget(key, 1, IPC_CREAT | 0666);
    if (result->id < 0) {
        perror("semget");
        sem__destroy(result);
        return 0;
    }

    return result;
}

void sem__destroy(sem_t self) {
    if (0 < self->id && semctl(self->id, 0, IPC_RMID) < 0) {
        perror("semctl");
    }

    free(self);
}

void sem__destroy_from_child(sem_t self) {
    free(self);
}

void sem__inc(sem_t self) {
    struct sembuf sb = {
        .sem_flg = SEM_UNDO,
        .sem_num = 0,
        .sem_op  = 1
    };
    semop(self->id, &sb, 1);
}

void sem__dec(sem_t self) {
    struct sembuf sb = {
        .sem_flg = SEM_UNDO,
        .sem_num = 0,
        .sem_op  = -1
    };
    semop(self->id, &sb, 1);
}

int sem__set(sem_t self, int value) {
    if (semctl(self->id, 0, SETVAL, value) < 0) {
        perror("semctl");
        return 1;
    }
    return 0;
}

int sem__get(sem_t self) {
    int semval = semctl(self->id, 0, GETVAL);
    if (semval == -1) {
        perror("semctl GETVAL");
        return -1;
    }
    return semval;
}

void sem__print(sem_t self) {
    // Get semaphore value
    int semval = semctl(self->id, 0, GETVAL);
    if (semval == -1) {
        perror("semctl GETVAL");
        exit(1);
    }
    printf("Semaphore value (semval): %d\n", semval);

    // Get number of processes waiting for semaphore to become zero (semzcnt)
    int semzcnt = semctl(self->id, 0, GETZCNT);
    if (semzcnt == -1) {
        perror("semctl GETZCNT");
        exit(1);
    }
    printf("Number of processes waiting for zero (semzcnt): %d\n", semzcnt);

    // Get number of processes waiting for semaphore to be incremented (semncnt)
    int semncnt = semctl(self->id, 0, GETNCNT);
    if (semncnt == -1) {
        perror("semctl GETNCNT");
        exit(1);
    }
    printf("Number of processes waiting for increment (semncnt): %d\n", semncnt);

    // Get the PID of the last process that performed an operation on the semaphore (sempid)
    int sempid = semctl(self->id, 0, GETPID);
    if (sempid == -1) {
        perror("semctl GETPID");
        exit(1);
    }
    printf("PID of last process that modified semaphore (sempid): %d\n", sempid);
}
