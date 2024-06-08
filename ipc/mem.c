#include "mem.h"
#include "seg.h"
#include "seg.c"

#include <unistd.h>
#include <sys/shm.h>
#include <stdlib.h>

int shared_mem__create(shared_mem_t self, size_t size, uint8_t nonnull_key_id) {
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

    int memory_id = shmget(key, size, 0666 | IPC_CREAT);
    if (memory_id < 0) {
        perror("shmget");
        return 1;
    }

    memory_slice_t memory_slice = (memory_slice_t) {
        .memory = shmat(memory_id, 0, 0),
        .size = size,
        .offset = 0
    };
    if (memory_slice.memory == (void*) -1) {
        perror("shmat");
        return 1;
    }

    self->id = memory_id;
    self->memory_slice = memory_slice;

    if (!seg__init(memory_slice)) {
        shared_mem__destroy(self);
        return 1;
    }

    if (sem__create(&self->sem, nonnull_key_id)) {
        shared_mem__destroy(self);
        return 1;
    }
    if (sem__set(&self->sem, 1)) {
        shared_mem__destroy(self);
        return 1;
    }

    return 0;
}

void shared_mem__destroy(shared_mem_t self) {
    if (0 < self->id && shmctl(self->id, IPC_RMID, 0) < 0) {
        perror("shmctl");
    }

    sem__destroy(&self->sem);
}

int shared_mem__map_to_address_space(shared_mem_t self) {
    void* mapped_addr = shmat(self->id, 0, 0);
    if (mapped_addr == (void*) -1) {
        perror("shmat");
        return 1;
    }
    self->memory_slice.memory = mapped_addr;
    return 0;
}

void* shared_mem__malloc(shared_mem_t self, size_t size) {
    sem__dec(&self->sem);
    void* result = seg__malloc(self->memory_slice, size);
    sem__inc(&self->sem);
    return result;
}

void* shared_mem__calloc(shared_mem_t self, size_t size) {
    sem__dec(&self->sem);
    void* result = seg__calloc(self->memory_slice, size);
    sem__inc(&self->sem);
    return result;
}

void* shared_mem__realloc(shared_mem_t self, void* old_ptr, size_t new_size) {
    sem__dec(&self->sem);
    void* result = seg__realloc(self->memory_slice, old_ptr, new_size);
    sem__inc(&self->sem);
    return result;
}

void shared_mem__free(shared_mem_t self, void* ptr) {
    sem__dec(&self->sem);
    seg__free(self->memory_slice, ptr);
    sem__inc(&self->sem);
}

void* shared_mem__locked_malloc(shared_mem_t self, size_t size) {
    return seg__malloc(self->memory_slice, size);
}

void* shared_mem__locked_calloc(shared_mem_t self, size_t size) {
    return seg__calloc(self->memory_slice, size);
}

void* shared_mem__locked_realloc(shared_mem_t self, void* old_ptr, size_t new_size) {
    return seg__realloc(self->memory_slice, old_ptr, new_size);
}

void shared_mem__locked_free(shared_mem_t self, void* ptr) {
    seg__free(self->memory_slice, ptr);
}

void shared_mem__lock(shared_mem_t self) {
    sem__dec(&self->sem);
}

void shared_mem__unlock(shared_mem_t self) {
    sem__inc(&self->sem);
}

void shared_mem__print(shared_mem_t self) {
    sem__dec(&self->sem);
    seg__for_each(self->memory_slice, &seg__print);
    sem__inc(&self->sem);
}
