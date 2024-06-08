#ifndef PROC_H
# define PROC_H

# include "mem.h"
# include "sem.h"
# include "msg.h"

# include <unistd.h>
# include <stddef.h>
# include <stdarg.h>

// Any process has access to these

int    shared__init(size_t shared_memory_size);
void   shared__deinit();

size_t shared__read(char* buffer, size_t buffer_size);
void   shared__write(const char* format, ...);

void*  shared__alloc(size_t size);
void*  shared__calloc(size_t);
void*  shared__realloc(void* old_ptr, size_t new_size);
void   shared__free(void* ptr);

// use when reading/writing to shared memory
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

proc_t proc__create(int (*fn)(proc_t));
void   proc__destroy(proc_t self);

proc_t proc__get_current();

size_t proc__read(proc_t self, char* buffer, size_t buffer_size);
void   proc__write(proc_t self, const char* format, ...);
void   proc__vwrite(proc_t self, const char* format, va_list ap);
int    proc__wait(proc_t self, int hang);

void   proc__print(proc_t self);

#endif // PROC_H
