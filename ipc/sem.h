#ifndef SEM_H
# define SEM_H

# include <stdint.h>

typedef struct sem {
    int id;
    int ref_count;
} *sem_t;

int sem__create(sem_t self, uint8_t nonnull_key_id);
void sem__destroy(sem_t self);

void sem__reset_ref_counter(sem_t self); // in new process to keep the sem atomic, reset its ref counter

void sem__lock(sem_t self);
void sem__unlock(sem_t self);

int sem__val(sem_t self); // -1 on failure, otherwise the value of the sem
int sem__zcnt(sem_t self); // -1 on failure, otherwise the n of processes waiting for zero
int sem__ncnt(sem_t self); // -1 on failure, otherwise the n of processes waiting for increment
int sem__pid(sem_t self); // -1 on failure, otherwise the pid of last process that modified the sem

void sem__print(sem_t self);

#endif // SEM_H
