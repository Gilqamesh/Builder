#ifndef SEGMENT_ALLOCATOR_H
# define SEGMENT_ALLOCATOR_H

# include <stdint.h>
# include <stdbool.h>
# include <stddef.h>

typedef struct memory_slice {
    void* memory;
    uint64_t size;
    uint64_t offset; // implementation specific, for now it's just offset
} memory_slice_t;

typedef void* seg_t;

// @brief marks memory as a seg
bool seg__init(memory_slice_t memory);

void* seg__malloc(memory_slice_t memory, size_t size);
void* seg__calloc(memory_slice_t memory, size_t size);
void* seg__realloc(memory_slice_t memory, void* ptr, size_t new_size);
void* seg__recalloc(memory_slice_t memory, void* ptr, size_t new_size);
void  seg__free(memory_slice_t memory, void* ptr);

// @brief prints segment
bool  seg__print(memory_slice_t memory, void* ptr);
void  seg__print_aux(memory_slice_t memory);

// @brief applies fn to each segment in memory while it returns true
void  seg__for_each(memory_slice_t memory, bool (*fn)(memory_slice_t memory, void* ptr));

#endif // SEGMENT_ALLOCATOR_H
