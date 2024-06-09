#ifndef PROC_H
# define PROC_H

# include "mem.h"
# include "sem.h"
# include "msg.h"

# include <unistd.h>
# include <stddef.h>
# include <stdarg.h>

/**
 * All of these operations are atomic across processes, but it's necessary to lock/unlock when readin/writing into shared memory
 * Proc:   creating new processes, read/write messages in between each other's msg queue
 * Shared: allocate, read/write into shared memory pool, send/write messages into global msg queue
*/ 

int    shared__init(size_t shared_memory_size);
void   shared__deinit(); // automatically frees unfreed memory

size_t shared__read(void* buffer, size_t buffer_size);
size_t shared__read_str(char* buffer, size_t buffer_size);
size_t shared__write(void* buffer, size_t buffer_size);
size_t shared__write_str(const char* format, ...);
size_t shared__vwrite_str(const char* format, va_list ap);

void*  shared__malloc(size_t size);
void*  shared__calloc(size_t size);
void*  shared__realloc(void* old_ptr, size_t new_size);
void   shared__free(void* ptr);

// Use these when reading/writing into shared memory
void   shared__lock();
void   shared__unlock();

void   shared__print();

// Processes are allocated from the shared memory pool
typedef struct proc {
    uint8_t          key_id;
    int              is_taken;
    struct proc*     parent;
    int              proc_depth; // n of parents
    size_t           children_size;
    size_t           children_top;
    struct proc**    children;
    pid_t            pid;
    struct msg_queue msg_queue;
} *proc_t;

proc_t proc__create(int (*fn)(void*), void*);
void   proc__destroy(proc_t self);

proc_t proc__get_current();

size_t proc__read(proc_t self, void* buffer, size_t buffer_size);
size_t proc__read_str(proc_t self, char* buffer, size_t buffer_size);
size_t proc__write(proc_t self, void* buffer, size_t buffer_size);
size_t proc__write_str(proc_t self, const char* format, ...);
size_t proc__vwrite_str(proc_t self, const char* format, va_list ap);
/**
 * @return
 *  -4  - continued by signal
 *  -3  - stopped by signal
 *  -2  - terminated by signal
 *  -1  - still running
 *  >=0 - return status code
*/
int    proc__wait(proc_t self, int hang);;

void   proc__print(proc_t self);

#endif // PROC_H
