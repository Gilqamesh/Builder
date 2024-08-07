#include "proc.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <wait.h>
#include <string.h>
#include <errno.h>

static struct {
    proc_t proc_cur;
    struct shared_mem memory;

    struct {
        struct proc main_proc;
        struct proc child_procs[192];
    } *shared;
} _;

static void cleanup_signal_handler(int signal);
static void cleanup_before_exit();

static void cleanup_signal_handler(int signal) {
    cleanup_before_exit();
    _exit(signal);
}

static void cleanup_before_exit() {
    if (_.shared && _.proc_cur == &_.shared->main_proc) {
        shared__deinit();
    }
}

int shared__init(size_t shared_memory_size) {
    struct sigaction sa = {
        .sa_handler = &cleanup_signal_handler
    };
    int signals_to_handle[] = { SIGTERM, SIGINT, SIGQUIT, SIGABRT, SIGHUP };

    for (size_t i = 0; i < sizeof(signals_to_handle) / sizeof(signals_to_handle[0]); ++i) {
        if (sigaction(signals_to_handle[i], &sa, NULL) < 0) {
            perror("sigaction");
            return 1;
        }
    }

    if (atexit(&cleanup_before_exit)) {
        shared__deinit();
        return 1;
    }

    uint8_t key_id = 1;

    if (shared_mem__create(&_.memory, shared_memory_size + sizeof(*_.shared), key_id++)) {
        return 1;
    }

    _.shared = shared__calloc(sizeof(*_.shared));
    if (!_.shared) {
        shared__deinit();
        return 1;
    }

    proc_t main_proc = &_.shared->main_proc;
    main_proc->key_id = key_id++;
    main_proc->is_taken = 1;
    if (msg_queue__create(&main_proc->msg_queue, main_proc->key_id)) {
        shared__deinit();
        return 1;
    }
    _.proc_cur = main_proc;

    for (size_t proc_index = 0; proc_index < sizeof(_.shared->child_procs) / sizeof(_.shared->child_procs[0]); ++proc_index) {
        _.shared->child_procs[proc_index].key_id = proc_index + key_id;
    }

    return 0;
}

void shared__deinit() {
    if (_.shared) {
        proc__destroy(&_.shared->main_proc);
        shared_mem__free(&_.memory, _.shared);
        _.shared = 0;
    }

    if (_.memory.memory_slice.memory) {
        shared_mem__print(&_.memory);
        shared_mem__destroy(&_.memory);
        _.memory.memory_slice.memory = 0;
    }
}

size_t shared__read(void* buffer, size_t buffer_size) {
    return proc__read(&_.shared->main_proc, buffer, buffer_size);
}

size_t shared__read_str(char* buffer, size_t buffer_size) {
    return proc__read_str(&_.shared->main_proc, buffer, buffer_size);
}

size_t shared__write(void* buffer, size_t buffer_size) {
    return proc__write(&_.shared->main_proc, buffer, buffer_size);
}

size_t shared__write_str(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    size_t result = shared__vwrite_str(format, ap);
    va_end(ap);

    return result;
}

size_t shared__vwrite_str(const char* format, va_list ap) {
    return proc__vwrite_str(&_.shared->main_proc, format, ap);
}

void* shared__malloc(size_t size) {
    return shared_mem__malloc(&_.memory, size);
}

void* shared__calloc(size_t size) {
    return shared_mem__calloc(&_.memory, size);
}

void* shared__realloc(void* old_ptr, size_t new_size) {
    return shared_mem__realloc(&_.memory, old_ptr, new_size);
}

void* shared__recalloc(void* old_ptr, size_t new_size) {
    return shared_mem__recalloc(&_.memory, old_ptr, new_size);
}

void shared__free(void* ptr) {
    return shared_mem__free(&_.memory, ptr);
}

void shared__lock() {
    shared_mem__lock(&_.memory);
}

void shared__unlock() {
    shared_mem__unlock(&_.memory);
}

void shared__print() {
    shared_mem__print(&_.memory);
    proc__print(&_.shared->main_proc);
}

proc_t proc__create(int (*fn)(void*), void* data) {
    shared__lock();

    proc_t result = 0;

    // look for available key
    for (size_t proc_index = 0; proc_index < sizeof(_.shared->child_procs) / sizeof(_.shared->child_procs[0]); ++proc_index) {
        if (!_.shared->child_procs[proc_index].is_taken) {
            _.shared->child_procs[proc_index].is_taken = 1;
            result = &_.shared->child_procs[proc_index];
            break ;
        }
    }

    if (!result) {
        goto err;
    }

    proc_t parent = proc__get_current();

    result->parent = parent;
    result->proc_depth = parent ? parent->proc_depth + 1 : 1;

    if (msg_queue__create(&result->msg_queue, result->key_id)) {
        goto err;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        goto err;
    } else if (pid == 0) {
        shared_mem__reset_ref_counter(&_.memory);
        _.proc_cur = result;
        shared__lock();
        shared__unlock();

        exit(fn(data));
    }
    result->pid = pid;

    if (parent) {
        if (parent->children_size <= parent->children_top) {
            parent->children_size = parent->children_size == 0 ? 4 : parent->children_size << 1;
            parent->children = shared_mem__realloc(&_.memory, parent->children, parent->children_size * sizeof(*parent->children));
            if (!parent->children) {
                goto err;
            }
        }
        parent->children[parent->children_top++] = result;
    }

    shared__unlock();

    return result;

err:
    proc__destroy(result);
    shared__unlock();
    return 0;
}

void proc__destroy(proc_t self) {
    shared__lock();

    if (!self->is_taken) {
        shared__unlock();
        return ;
    }

    assert(getpid() != self->pid && "only our parent should be able to destroy us");

    self->proc_depth = 0;
    self->is_taken = 0;
    if (self->msg_queue.id) {
        msg_queue__destroy(&self->msg_queue);
        self->msg_queue.id = 0;
    }

    // remove from parent
    if (self->parent) {
        for (size_t child_index = 0; child_index < self->parent->children_top; ++child_index) {
            if (self->parent->children[child_index] == self) {
                while (child_index + 1 < self->parent->children_top) {
                    self->parent->children[child_index] = self->parent->children[child_index + 1];
                    ++child_index;
                }
                --self->parent->children_top;
                // todo: free some space
                break ;
            }
        }
        self->parent = 0;
    }

    if (0 < self->pid) {
        if (kill(self->pid, SIGTERM) < 0) {
            perror("kill");
        }
        if (waitpid(self->pid, 0, 0) < 0) {
            perror("waitpid");
        }
        self->pid = 0;
    }

    for (size_t child_index = 0; child_index < self->children_top; ++child_index) {
        proc__destroy(self->children[child_index]);
    }

    if (self->children) {
        shared_mem__free(&_.memory, self->children);
        self->children_top = 0;
        self->children_size = 0;
        self->children = 0;
    }

    shared__unlock();
}

proc_t proc__get_current() {
    return _.proc_cur;
}

size_t proc__read(proc_t self, void* buffer, size_t buffer_size) {
    shared__lock();

    if (!self->is_taken) {
        shared__unlock();
    }

    size_t result = msg_queue__read(&self->msg_queue, buffer, buffer_size);

    shared__unlock();

    return result;
}

size_t proc__read_str(proc_t self, char* buffer, size_t buffer_size) {
    shared__lock();

    if (!self->is_taken) {
        shared__unlock();
    }

    size_t result = msg_queue__read_str(&self->msg_queue, buffer, buffer_size);

    shared__unlock();

    return result;
}

size_t proc__write(proc_t self, void* buffer, size_t buffer_size) {
    shared__lock();

    if (!self->is_taken) {
        shared__unlock();
    }

    size_t result = msg_queue__write(&self->msg_queue, buffer, buffer_size);

    shared__unlock();

    return result;
}

size_t proc__write_str(proc_t self, const char* format, ...) {
    shared__lock();

    if (!self->is_taken) {
        shared__unlock();
    }

    va_list ap;
    va_start(ap, format);
    size_t result = msg_queue__vwrite_str(&self->msg_queue, format, ap);
    va_end(ap);

    shared__unlock();

    return result;
}

size_t proc__vwrite_str(proc_t self, const char* format, va_list ap) {
    shared__lock();

    if (!self->is_taken) {
        shared__unlock();
    }

    size_t result = msg_queue__vwrite_str(&self->msg_queue, format, ap);

    shared__unlock();

    return result;
}

int proc__wait(proc_t self, int hang) {
    shared__lock();

    if (!self->is_taken) {
        shared__unlock();
    }

    if (self->pid == 0) {
        // no child process to wait for
        shared__unlock();
        return 0;
    }

    assert(getpid() != self->pid && "we are the child process, why are we waiting for ourself?");
    
int result = 0;
    int wstatus = 0;
    pid_t waited_pid = waitpid(self->pid, &wstatus, hang ? 0 : WNOHANG);
    if (waited_pid < 0) {
        proc__write_str(self, "waitpid: %s", strerror(errno));
        self->pid = 0;
    } else if (0 < waited_pid) {
        assert(waited_pid == self->pid);
        if (WIFEXITED(wstatus)) {
            wstatus = WEXITSTATUS(wstatus);
            proc__write_str(self, "terminated normally, exit status code: %d", wstatus);
            result = wstatus;
            assert(result >= 0);
        } else if (WIFSIGNALED(wstatus)) {
            result = -2;
            int sig = WTERMSIG(wstatus);
            proc__write_str(self, "terminated by signal: %s", strsignal(sig));
        } else if (WIFSTOPPED(wstatus)) {
            result = -3;
            int sig = WSTOPSIG(wstatus);
            proc__write_str(self, "stopped by signal: %s", strsignal(sig));
        } else if (WIFCONTINUED(wstatus)) {
            result = -4;
            proc__write_str(self, "continued by signal: %s", strsignal(SIGCONT));
        } else {
            assert(0);
        }
        self->pid = 0;
    } else {
        result = -1;
    }

    shared__unlock();

    return result;
}

void proc__print(proc_t self) {
    shared__lock();

    if (!self->is_taken) {
        shared__unlock();
    }

    msg_queue__print(&self->msg_queue);

    shared__unlock();
}
