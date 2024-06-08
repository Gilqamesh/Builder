#ifndef SEM_H
# define SEM_H

# include <stdint.h>

typedef struct sem {
    int id;
} *sem_t;

int sem__create(sem_t self, uint8_t nonnull_key_id);
void sem__destroy(sem_t self);

void sem__inc(sem_t self);
void sem__dec(sem_t self);

int sem__set(sem_t self, int value);

/**
 * Returns -1 on failure, otherwise the value of the semaphore
*/
int sem__get(sem_t self);

/**
 * Prints useful information about the semaphore
*/
int sem__print(sem_t self);

#endif // SEM_H
