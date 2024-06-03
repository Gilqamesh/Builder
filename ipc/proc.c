#include "proc.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <wait.h>

proc_t proc__create(proc_t parent) {
    proc_t result = calloc(1, sizeof(*result));
    if (!result) {
        return 0;
    }

    result->has_parent = parent ? 1 : 0;
    if (result->has_parent) {
        result->shared_memory = parent->shared_memory;
        result->sem = parent->sem;
        result->msg_queue = parent->msg_queue;
    } else {
        result->shared_memory = shared_mem__create(4096 * 16);
        if (!result->shared_memory) {
            proc__destroy(result);
            return 0;
        }
        result->sem = sem__create();
        if (!result->sem) {
            proc__destroy(result);
            return 0;
        }
        if (sem__set(result->sem, 1)) {
            proc__destroy(result);
            return 0;
        }
        result->msg_queue = msg_queue__create();
        if (!result->msg_queue) {
            proc__destroy(result);
            return 0;
        }
    }

    return result;
}

void proc__destroy(proc_t self) {
    if (0 < self->pid_child) {
        if (kill(self->pid_child, SIGTERM) < 0) {
            perror("kill");
        }
    }
    if (self->has_parent) {
        if (self->shared_memory) {
            shared_mem__destroy_from_child(self->shared_memory);
        }
        if (self->sem) {
            sem__destroy_from_child(self->sem);
        }
        if (self->msg_queue) {
            msg_queue__destroy_from_child(self->msg_queue);
        }
    } else {
        if (self->shared_memory) {
            shared_mem__destroy(self->shared_memory);
        }
        if (self->sem) {
            sem__destroy(self->sem);
        }
        if (self->msg_queue) {
            msg_queue__destroy(self->msg_queue);
        }
    }
    free(self);
}

int proc__run(proc_t self, void (*fn)(proc_t)) {
    if (0 < self->pid_child) {
        if (kill(self->pid_child, SIGTERM) < 0) {
            perror("kill");
        }
    }

    self->pid_child = fork();
    if (self->pid_child < 0) {
        perror("fork");
        self->pid_child = 0;
        return 1;
    } else if (self->pid_child == 0) {
        self->has_parent = 1;
        fn(self);
        proc__destroy(self);
        exit(0);
    }

    return 0;
}

size_t proc__read(proc_t self, char* buffer, size_t buffer_size) {
    sem__dec(self->sem);
    size_t bytes_read = msg_queue__read(self->msg_queue, buffer, buffer_size);
    sem__inc(self->sem);
    return bytes_read;
}

void proc__write(proc_t self, const char* format, ...) {
    sem__dec(self->sem);
    va_list ap;
    va_start(ap, format);
    msg_queue__vwrite(self->msg_queue, format, ap);
    va_end(ap);
    sem__inc(self->sem);
}

int proc__wait(proc_t self, int hang) {
    if (self->pid_child == 0) {
        // no child process to wait for
        return 1;
    }

    int wstatus = 0;
    pid_t waited_pid = waitpid(self->pid_child, &wstatus, hang ? 0 : WNOHANG);
    if (waited_pid < 0) {
        perror("waitpid");
        self->pid_child = 0;
        return 1;
    } else if (0 < waited_pid) {
        assert(waited_pid == self->pid_child);
        if (WIFEXITED(wstatus)) {
            wstatus = WEXITSTATUS(wstatus);
            // terminated normally, wstatus contains exit code
        } else if (WIFSIGNALED(wstatus)) {
            int sig = WTERMSIG(wstatus);
            // terminated by signal
            (void) sig;
        } else if (WIFSTOPPED(wstatus)) {
            int sig = WSTOPSIG(wstatus);
            // stopped by signal
            (void) sig;
        } else if (WIFCONTINUED(wstatus)) {
            // continued by signal
        } else {
            assert(0);
        }
        self->pid_child = 0;
    } else {
        // still running
        return 1;
    }

    return 0;
}

void* proc__alloc(proc_t self, size_t size) {
    sem__dec(self->sem);
    void* result = shared_mem__malloc(self->shared_memory, size);
    sem__inc(self->sem);
    return result;
}

void* proc__calloc(proc_t self, size_t size) {
    sem__dec(self->sem);
    void* result = shared_mem__calloc(self->shared_memory, size);
    sem__inc(self->sem);
    return result;
}

void* proc__realloc(proc_t self, void* old_ptr, size_t new_size) {
    sem__dec(self->sem);
    void* result = shared_mem__realloc(self->shared_memory, old_ptr, new_size);
    sem__inc(self->sem);
    return result;
}

void proc__free(proc_t self, void* ptr) {
    sem__dec(self->sem);
    shared_mem__free(self->shared_memory, ptr);
    sem__inc(self->sem);
}

void proc__lock_shared(proc_t self) {
    sem__dec(self->sem);
}

void proc__unlock_shared(proc_t self) {
    sem__inc(self->sem);
}

void proc__print_shared(proc_t self) {
    sem__dec(self->sem);
    shared_mem__print(self->shared_memory);
    msg_queue__print(self->msg_queue);
    sem__print(self->sem);
    sem__inc(self->sem);
}
