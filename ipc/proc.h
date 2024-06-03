#ifndef PROC_H
# define PROC_H

# include "mem.h"
# include "sem.h"
# include "msg.h"

# include <unistd.h>
# include <stddef.h>
# include <stdarg.h>

typedef struct proc {
    int          has_parent;
    pid_t        pid_child;
    shared_mem_t shared_memory;
    sem_t        sem;
    msg_queue_t  msg_queue;
} *proc_t;

proc_t proc__create(proc_t parent);
void   proc__destroy(proc_t self);

int    proc__run(proc_t self, void (*fn)(proc_t));
size_t proc__read(proc_t self, char* buffer, size_t buffer_size);
void   proc__write(proc_t self, const char* format, ...);
int    proc__wait(proc_t self, int hang);

void*  proc__alloc(proc_t self, size_t size);
void*  proc__calloc(proc_t self, size_t);
void*  proc__realloc(proc_t self, void* old_ptr, size_t new_size);
void   proc__free(proc_t self, void* ptr);

void   proc__lock_shared(proc_t self);
void   proc__unlock_shared(proc_t self);

void   proc__print_shared(proc_t self);

#endif // PROC_H
