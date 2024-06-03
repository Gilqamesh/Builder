#include "mem.h"
#include "seg.h"
#include "seg.c"

#include <unistd.h>
#include <sys/shm.h>
#include <stdlib.h>

shared_mem_t shared_mem__create(size_t size) {
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

    int memory_id = shmget(key, size, 0666 | IPC_CREAT);
    if (memory_id < 0) {
        perror("shmget");
        return 0;
    }

    memory_slice_t memory_slice = (memory_slice_t) {
        .memory = shmat(memory_id, 0, 0),
        .size = size,
        .offset = 0
    };
    if (memory_slice.memory == (void*) -1) {
        perror("shmat");
        return 0;
    }

    shared_mem_t result = calloc(1, sizeof(*result));
    if (!result) {
        return 0;
    }

    result->id = memory_id;
    result->memory_slice = memory_slice;

    if (!seg__init(memory_slice)) {
        shared_mem__destroy(result);
        return 0;
    }

    return result;
}

void shared_mem__destroy(shared_mem_t self) {
    if (0 < self->id && shmctl(self->id, IPC_RMID, 0) < 0) {
        perror("shmctl");
    }
    free(self);
}

void shared_mem__destroy_from_child(shared_mem_t self) {
    if (self->memory_slice.memory && shmdt(self->memory_slice.memory) < 0) {
        perror("shmdt");
    }
    free(self);
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

    return seg__malloc(self->memory_slice, size);
}

void* shared_mem__calloc(shared_mem_t self, size_t size) {
    return seg__calloc(self->memory_slice, size);
}

void* shared_mem__realloc(shared_mem_t self, void* old_ptr, size_t new_size) {
    return seg__realloc(self->memory_slice, old_ptr, new_size);
}

void shared_mem__free(shared_mem_t self, void* ptr) {
    seg__free(self->memory_slice, ptr);
}

void shared_mem__print(shared_mem_t self) {
    seg__for_each(self->memory_slice, &seg__print);
}
