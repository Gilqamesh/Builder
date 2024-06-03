#ifndef SEM_H
# define SEM_H

typedef struct sem {
    int id;
} *sem_t;

sem_t sem__create();
void sem__destroy(sem_t self);
void sem__destroy_from_child(sem_t self);

void sem__inc(sem_t self);
void sem__dec(sem_t self);

/**
 * Returns 1 on failure, otherwise 0
*/
int sem__set(sem_t self, int value);

/**
 * Returns -1 on failure, otherwise the value of the semaphore
*/
int sem__get(sem_t self);

/**
 * Prints useful information about the semaphore
*/
void sem__print(sem_t self);

#endif // SEM_H
