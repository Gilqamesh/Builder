#ifndef MEM_H
# define MEM_H

# include <stddef.h>
# include "seg.h"

typedef struct shared_mem {
    int   id;
    memory_slice_t memory_slice;
} *shared_mem_t;

shared_mem_t shared_mem__create(size_t size);
void shared_mem__destroy(shared_mem_t self);
void shared_mem__destroy_from_child(shared_mem_t self);

// No need to call after fork, as child process inherits shared mem
int shared_mem__map_to_address_space(shared_mem_t self);

void* shared_mem__malloc(shared_mem_t self, size_t size);
void* shared_mem__calloc(shared_mem_t self, size_t size);
void* shared_mem__realloc(shared_mem_t self, void* old_ptr, size_t new_size);
void  shared_mem__free(shared_mem_t self, void* ptr);

void shared_mem__print(shared_mem_t self);

#endif // MEM_H
