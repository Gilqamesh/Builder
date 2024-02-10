#include "builder.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

static size_t children_pid_top = 0;
static pid_t  children_pid[256];

#define ARRAY_SIZE(array) (sizeof(array)/sizeof((array)[0]))
#define ARRAY_ENSURE_TOP(array_p, array_top, array_size) do { \
    if ((array_top) >= (array_size)) { \
        if ((array_size) == 0) { \
            (array_size) = 8; \
            (array_p) = malloc((array_size) * sizeof((array_p)[0])); \
        } else { \
            (array_size) <<= 1; \
            (array_p) = realloc((array_p), (array_size) * sizeof((array_p)[0])); \
            } \
    } \
} while (0)

struct obj {
    int      dependencies_top;
    int      dependencies_size;
    obj_t*   dependencies;

    time_t   time;
    
    // used for anything, clear once done using it
    int      transient_flag;

    void     (*check)(obj_t self);
    void     (*invoke)(obj_t self);
    void     (*print)(obj_t self);
    void     (*destroy)(obj_t self);
};

typedef struct obj_file {
    struct obj base;
    char*      path;
} *obj_file_t;

typedef struct obj_process {
    struct obj base;
    char*      cmd_line;
    pid_t      pid;
} *obj_process_t;

static void obj__add_dependency(obj_t self, obj_t dependency);
static void obj__check_for_cyclic_dependency(obj_t self);
static int  obj__check_for_cyclic_dependency_helper(obj_t self);
static void obj__print_helper(obj_t self, int depth);

static void obj__check_process(obj_t self);
static void obj__check_file(obj_t self);

static void obj__invoke_process(obj_t self);

static void obj__print_process(obj_t self);
static void obj__print_file(obj_t self);

static void obj__destroy_process(obj_t self);
static void obj__destroy_file(obj_t self);

static void children_pid__signal_handler(int signal);
static void children_pid__add_child(pid_t pid);
static void children_pid__remove_child(pid_t pid);

static void obj__add_dependency(obj_t self, obj_t dependency) {    
    if (dependency->transient_flag == 1) {
        for (int dependency_index = 0; dependency_index < dependency->dependencies_top; ++dependency_index) {
            obj_t dep = dependency->dependencies[dependency_index];
            obj__add_dependency(self, dep);
        }
        free(dependency);
        return ;
    }

    assert(self != dependency);
    ARRAY_ENSURE_TOP(self->dependencies, self->dependencies_top, self->dependencies_size);
    self->dependencies[self->dependencies_top++] = dependency;

    if (self->transient_flag == 0) {
        obj__check_for_cyclic_dependency(self);
    }
}

static void obj__check_for_cyclic_dependency(obj_t self) {
    assert(self->transient_flag == 0);
    if (obj__check_for_cyclic_dependency_helper(self)) {
        obj__print(self);
        assert(0);
    }
}

static int obj__check_for_cyclic_dependency_helper(obj_t self) {
    if (self->transient_flag) {
        self->transient_flag = 0;
        printf("Cyclic dependences detected between:\n");
        return 1;
    }
    self->transient_flag = 1;

    for (int dependency_index = 0; dependency_index < self->dependencies_top; ++dependency_index) {
        obj_t dependency = self->dependencies[dependency_index];
        if (obj__check_for_cyclic_dependency_helper(dependency)) {
            self->transient_flag = 0;
            printf("  ");
            self->print(self);
            return 1;
        }
    }

    self->transient_flag = 0;
    
    return 0;
}

static void obj__print_helper(obj_t self, int depth) {
    printf("%*c", depth * 2, ' ');
    self->print(self);

    if (self->transient_flag) {
        return ;
    }

    self->transient_flag = 1;

    for (int dependency_index = 0; dependency_index < self->dependencies_top; ++dependency_index) {
        obj_t dependency = self->dependencies[dependency_index];
        obj__print_helper(dependency, depth + 1);
    }

    self->transient_flag = 0;
}

static void obj__check_process(obj_t self) {
    obj_process_t process = (obj_process_t) self;
    if (process->pid > 0) {
        int wstatus = -1;
        pid_t file_pid = waitpid(process->pid, &wstatus, WNOHANG);
        if (file_pid == -1) {
            children_pid__remove_child(process->pid);
            process->pid = 0;
            perror(0);
        } else if (file_pid > 0) {
            assert(file_pid == process->pid);
            children_pid__remove_child(process->pid);
            if (WIFEXITED(wstatus)) {
                wstatus = WEXITSTATUS(wstatus);
                if (!wstatus) {
                    self->time = time(0);
                } else {
                    printf("Process returned with %d status code\n", wstatus);
                }
            } else if (WIFSIGNALED(wstatus)) {
                int sig = WTERMSIG(wstatus);
                printf("Process was terminated by signal: %d %s\n", sig, strsignal(sig));
            } else if (WIFSTOPPED(wstatus)) {
                int sig = WSTOPSIG(wstatus);
                printf("Process was stopped by signal: %d %s\n", sig, strsignal(sig));
            } else if (WIFCONTINUED(wstatus)) {
                printf("Process was continued by signal %d %s\n", SIGCONT, strsignal(SIGCONT));
            }
            process->pid = 0;
        }
    }
}

static void obj__check_file(obj_t self) {
    struct stat file_info;
    if (stat(((obj_file_t) self)->path, &file_info) == -1) {
        return ;
    }

    if (self->time < file_info.st_mtime) {
        self->time = file_info.st_mtime;
    }
}

static void obj__invoke_process(obj_t self) {
    obj_process_t process = (obj_process_t) self;
    if (process->pid > 0) {
        kill(process->pid, SIGTERM);
        waitpid(process->pid, 0, 0);
        children_pid__remove_child(process->pid);
    }

    process->pid = fork();
    if (process->pid < 0) {
        perror(0);
        exit(1);
    }
    if (process->pid == 0) {
        execl("/usr/bin/sh", "sh", "-c", process->cmd_line, NULL);
        perror(0);
        exit(1);
    }
    children_pid__add_child(process->pid);
    self->print(self);

}

static void obj__print_process(obj_t self) {
    obj_process_t process = (obj_process_t) self;
    printf("/usr/bin/sh -c \"%s\"\n", process->cmd_line);
}

static void obj__print_file(obj_t self) {
    obj_file_t file = (obj_file_t) self;
    printf("%s\n", file->path);
}

static void obj__destroy_process(obj_t self) {
    (void) self;
    assert(0 && "todo: implement");
}
static void obj__destroy_file(obj_t self) {
    (void) self;
    assert(0 && "todo: implement");
}

static void children_pid__signal_handler(int signal) {
    if (signal == SIGTERM) {
        for (size_t i = 0; i < ARRAY_SIZE(children_pid); ++i) {
            if (children_pid[i]) {
                kill(children_pid[i], SIGTERM);
                waitpid(children_pid[i], 0, 0);
            }
        }
        exit(1);
    }
}

static void children_pid__add_child(pid_t pid) {
    assert(children_pid_top++ < ARRAY_SIZE(children_pid));
    for (size_t i = 0; i < ARRAY_SIZE(children_pid); ++i) {
        if (children_pid[i] == 0) {
            children_pid[i] = pid;
            break ;
        }
    }
}

static void children_pid__remove_child(pid_t pid) {
    assert(children_pid_top-- > 0);
    for (size_t i = 0; i < ARRAY_SIZE(children_pid); ++i) {
        if (children_pid[i] == pid) {
            children_pid[i] = 0;
            break ;
        }
    }
}

void builder__init() {
    struct sigaction act = { 0 };
    act.sa_handler = &children_pid__signal_handler;
    sigaction(SIGTERM, &act, 0);
}

obj_t obj__file(const char* path, ...) {
    obj_file_t result = calloc(1, sizeof(*result));
    if (!result) {
        return 0;
    }

    result->base.check   = &obj__check_file;
    result->base.invoke  = &obj__check_file;
    result->base.print   = &obj__print_file;
    result->base.destroy = &obj__destroy_file;

    assert(path);
    va_list ap;
    va_start(ap, path);
    va_list ap_copy;
    va_copy(ap_copy, ap);
    int len = vsnprintf(0, 0, path, ap_copy);
    assert(len >= 0);
    va_end(ap_copy);
    result->path = malloc(len + 1);
    vsnprintf(result->path, len + 1, path, ap);
    va_end(ap);

    // result->base.check((obj_t) result);

    return (obj_t) result;
}

const char* obj__file_path(obj_t self) {
    return ((obj_file_t) self)->path;
}

obj_t obj__process(obj_t opt_dep_in, obj_t opt_dep_out, const char* cmd_line, ...) {
    obj_process_t result = calloc(1, sizeof(*result));
    if (!result) {
        return 0;
    }

    result->base.check   = &obj__check_process;
    result->base.invoke  = &obj__invoke_process;
    result->base.print   = &obj__print_process;
    result->base.destroy = &obj__destroy_process;

    assert(cmd_line);
    va_list ap;
    va_start(ap, cmd_line);
    va_list ap_copy;
    va_copy(ap_copy, ap);
    int len = vsnprintf(0, 0, cmd_line, ap_copy);
    assert(len >= 0);
    va_end(ap_copy);
    result->cmd_line = malloc(len + 1);
    vsnprintf(result->cmd_line, len + 1, cmd_line, ap);
    va_end(ap);

    if (opt_dep_out) {
        obj__add_dependency(opt_dep_out, (obj_t) result);
    }

    if (opt_dep_in) {
        obj__add_dependency((obj_t) result, opt_dep_in);
    }

    return opt_dep_out ? opt_dep_out : (obj_t) result;
}

void obj__destroy(obj_t self) {
    self->destroy(self);
}

obj_t obj__dependencies(obj_t dep0, ... /*, 0*/) {
    obj_t result = calloc(1, sizeof(*result));
    result->transient_flag = 1;

    obj_t dep = dep0;
    obj__add_dependency(result, dep);

    va_list ap;
    va_start(ap, dep0);

    dep = va_arg(ap, obj_t);
    while (dep) {
        obj__add_dependency(result, dep);
        dep = va_arg(ap, obj_t);
    }

    va_end(ap);

    return result;
}

void obj__print(obj_t self) {
    printf("  ---==== Obj Graph ====---\n");
    obj__print_helper(self, 0);
    printf("  =============================\n");
}

void obj__build(obj_t self) {
    time_t time = self->time;
    int dont_invoke = 0;
    for (int dependency_index = 0; dependency_index < self->dependencies_top; ++dependency_index) {
        obj_t dependency = self->dependencies[dependency_index];
        time_t prev_time = dependency->time;
        obj__build(dependency);
        if (dependency->time == 0) {
            dont_invoke = 1;
            continue ;
        }   

        if (prev_time < dependency->time) {
            time = dependency->time;
        }
    }

    self->check(self);

    if (!dont_invoke && self->time < time) {
        self->invoke(self);
    }
}
