#include "builder.h"

#include "ipc/proc.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define ARRAY_SIZE(array) (sizeof(array)/sizeof((array)[0]))

static struct {
    obj_t           engine_time;
    obj_t           oscillator_200ms;
    obj_t           oscillator_10s;
    obj_t           c_compiler;
    obj_t           builder_h;
    obj_t           builder_c;
    obj_t           builder_o;

    double          g_tick_resolution;
    double          g_time_init;

    // shared stuff
    // objects live in the main process
    size_t objects_top;
    size_t objects_size;
    obj_t* objects;
} *_;

typedef struct obj_file_modified {
    struct obj base;
    char*      path;
} *obj_file_modified_t;

typedef struct obj_exec {
    struct obj base;
    char*      cmd_line;
} *obj_exec_t;

typedef struct obj_oscillator {
    struct obj base;
    double     periodicity_ms;
    double     time_ran_mod;
} *obj_oscillator_t;

typedef struct obj_time {
    struct obj base;
} *obj_time_t;

typedef struct obj_process {
    struct obj base;
    proc_t     proc;
    int        success_status_code;
} *obj_process_t;

typedef struct obj_list {
    struct obj base;
} *obj_list_t;

static void obj__check_for_cyclic_input(obj_t self);
static void obj__start_proxy(obj_t self);

static void obj__run_exec(obj_t self);
static void obj__run_file_modified(obj_t self);
static void obj__run_oscillator(obj_t self);
static void obj__run_time(obj_t self);
static void obj__run_process(obj_t self);
static void obj__run_list(obj_t self);

static void obj__describe_short_exec(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_file_modified(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_oscillator(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_time(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_process(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_list(obj_t self, char* buffer, int buffer_size);

static void obj__describe_long_exec(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_file_modified(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_oscillator(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_time(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_process(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_list(obj_t self, char* buffer, int buffer_size);

static void obj__destroy_exec(obj_t self);
static void obj__destroy_file_modified(obj_t self);
static void obj__destroy_oscillator(obj_t self);
static void obj__destroy_time(obj_t self);
static void obj__destroy_process(obj_t self);
static void obj__destroy_list(obj_t self);

static void obj__print_input_graph_helper(obj_t self, int depth) {
    obj__print(self, "");

    if (self->transient_flag) {
        return ;
    }

    self->transient_flag = 1;

    for (size_t input_index = 0; input_index < self->inputs_top; ++input_index) {
        obj_t input = self->inputs[input_index];
        obj__print_input_graph_helper(input, depth + 1);
    }

    self->transient_flag = 0;
}

static int obj__check_for_cyclic_input_helper(obj_t self) {
    if (self->transient_flag) {
        self->transient_flag = 0;
        obj__print(self, "Cyclic input detected between:\n");
        return 1;
    }
    self->transient_flag = 1;

    for (size_t input_index = 0; input_index < self->inputs_top; ++input_index) {
        obj_t input = self->inputs[input_index];
        if (obj__check_for_cyclic_input_helper(input)) {
            self->transient_flag = 0;
            obj__print(self, "");
            return 1;
        }
    }

    self->transient_flag = 0;
    
    return 0;
}

static void obj__check_for_cyclic_input(obj_t self) {
    assert(self->transient_flag == 0);
    if (obj__check_for_cyclic_input_helper(self)) {
        obj__print_input_graph_helper(self, 0);
        assert(0);
    }
}

static void obj__run_exec(obj_t self) {
    obj__set_start(self, builder__get_time_stamp());
    obj_exec_t exec = (obj_exec_t) self;
    if (execl("/usr/bin/sh", "sh", "-c", exec->cmd_line, NULL)) {
        perror("execl");
        kill(getpid(), SIGTERM);
    }
    obj__set_finish(self, builder__get_time_stamp());
}

static void obj__run_file_modified(obj_t self) {
    obj_file_modified_t file_modified = (obj_file_modified_t) self;

    const double time_cur = builder__get_time_stamp();
    struct stat file_info;
    if (stat(file_modified->path, &file_info) == -1) {
        obj__set_start(self, time_cur);
        obj__set_fail(self, time_cur);
        obj__set_finish(self, time_cur);
    } else {
        struct attr attr = obj__get_attr(self);
        if (attr.time_success == 0.0 || 0 < difftime(file_info.st_mtime, (time_t) attr.time_success)) {
            const double time_modified = (double) file_info.st_mtime;
            obj__set_start(self, time_modified);
            obj__set_success(self, time_modified);
            obj__set_finish(self, time_modified);
        }
    }
}

static void obj__run_oscillator(obj_t self) {
    const double time_cur = builder__get_time_stamp();

    obj__set_start(self, time_cur);

    struct attr attr = obj__get_attr(self);

    obj_oscillator_t oscillator = (obj_oscillator_t) self;
    if (attr.time_success == 0.0) {
        obj__set_success(self, time_cur);
        obj__set_finish(self, time_cur);
        return ;
    }

    const double periodicity_s = oscillator->periodicity_ms / 1000.0;
    const double minimum_periodicity_s = 0.1;
    if (periodicity_s < minimum_periodicity_s) {
        obj__set_success(self, time_cur);
        obj__set_finish(self, time_cur);
        return ;
    }

    const double epsilon_s = minimum_periodicity_s / 10.0; // to avoid modding down a whole period
    const double delta_time_last_successful_run_s = attr.time_success - g_time_init + epsilon_s;
    const double last_successful_run_mod_s = delta_time_last_successful_run_s - fmod(delta_time_last_successful_run_s, periodicity_s);
    const double delta_time_current_s = builder__get_time_stamp() - g_time_init;
    if (last_successful_run_mod_s + periodicity_s < delta_time_current_s) {
        obj__set_success(self, time_cur);
        obj__set_finish(self, time_cur);
    }
}

static void obj__run_time(obj_t self) {
    const double time_cur = builder__get_time_stamp();
    obj__set_start(self, time_cur);
    obj__set_success(self, time_cur);
    obj__set_finish(self, time_cur);
}

static void obj__process_child_fn(void* data) {
    obj_t self = (obj_t) data;
    
    // need new memory locks
    for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
        obj_t output = self->outputs[output_index];
        output->run(output);
    }

    struct attr attr = obj__get_attr(self);
    exit(attr.is_running || attr.time_success <= attr.time_fail);
}

static void obj__start_proxy(obj_t self) {
    const double time_cur = builder__get_time_stamp();


    struct attr attr = obj__get_attr(self);

    if (self->is_process) {
        obj_process_t obj_process = (obj_process_t) self;
        if (obj_process->proc) {
            // what if we destroy child processes here?
            proc__destroy_locked(obj_process->proc);
            obj__set_finish(self, time_cur);
            for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
                obj_t output = self->outputs[output_index];
                obj__set_finish(output, time_cur);
            }
        }
        obj__set_start(self, time_cur);
        for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
            obj_t output = self->outputs[output_index];
            obj__set_start(output, time_cur);
        }
        obj_process->proc = proc__create(&obj__process_child_fn, self);
        if (!obj_process->proc) {
            obj__set_fail(self, time_cur);
            obj__set_finish(self, time_cur);
        }
    } else {
        assert(0 && "not recognized proxy type");
    }
}

static void obj__run_process(obj_t self) {
    shared__lock();

    obj_process_t obj_process = (obj_process_t) self;
    if (!obj_process->proc) {
        shared__unlock();
        return ;
    }

    char buffer[256];
    if (proc__read(obj_process->proc, buffer, sizeof(buffer))) {
        printf("%s\n", buffer);
    }

    int wstatus = proc__wait(obj_process->proc, 0);
    if (wstatus == -4) {
        // continued, do nothing
    } else if (wstatus == -3) {
        // stopped, do nothing
    } else if (wstatus == -2) {
        // terminated by signal, do nothing
    } else if (wstatus == -1) {
        // still running, do nothing
    } else {
        assert(0 <= wstatus);
        const double time_cur = builder__get_time_stamp();
        obj__set_finish(self, time_cur);
        if (obj_process->success_status_code == wstatus) {
            obj__set_success(self, time_cur);
        } else {
            obj__set_fail(self, time_cur);
        }
        proc__destroy_locked(obj_process->proc);
        obj_process->proc = 0;
    }

    shared__unlock();
}

static void obj__run_list(obj_t self) {
    (void) self;
    assert(0 && "lists should be used as temporary containers to create programs with");
}

static void obj__describe_short_exec(obj_t self, char* buffer, int buffer_size) {
    obj_exec_t exec = (obj_exec_t) self;
    snprintf(
        buffer, buffer_size,
        "/usr/bin/sh -c \"%s\"",
        exec->cmd_line
    );
}

static void obj__describe_short_file_modified(obj_t self, char* buffer, int buffer_size) {
    obj_file_modified_t file = (obj_file_modified_t) self;
    snprintf(buffer, buffer_size, "%s", file->path);
}

static void obj__describe_short_oscillator(obj_t self, char* buffer, int buffer_size) {
    obj_oscillator_t oscillator = (obj_oscillator_t) self;
    snprintf(buffer, buffer_size, "%.3lf [ms]", oscillator->periodicity_ms);
}

static void obj__describe_short_time(obj_t self, char* buffer, int buffer_size) {
    (void) self;
    snprintf(buffer, buffer_size, "TIME");
}

static void obj__describe_short_process(obj_t self, char* buffer, int buffer_size) {
    obj_process_t obj_process = (obj_process_t) self;
    snprintf(buffer, buffer_size, "pid: %d", obj_process->pid);
}

static void obj__describe_short_list(obj_t self, char* buffer, int buffer_size) {
    assert(0 && "lists should be used as temporary containers to create programs with");
    (void) self;
    (void) buffer;
    (void) buffer_size;
}

static void obj__describe_long_exec(obj_t self, char* buffer, int buffer_size) {
    obj_exec_t exec = (obj_exec_t) self;
    snprintf(buffer, buffer_size,
        "EXEC"
        "\n%s",
        exec->cmd_line
    );
}

static void obj__describe_long_file_modified(obj_t self, char* buffer, int buffer_size) {
    obj_file_modified_t file = (obj_file_modified_t) self;
    snprintf(
        buffer, buffer_size,
        "FILE MODIFIED"
        "\n%s",
        file->path
    );
}

static void obj__describe_long_oscillator(obj_t self, char* buffer, int buffer_size) {
    obj_oscillator_t oscillator = (obj_oscillator_t) self;
    snprintf(buffer, buffer_size, "OSCILLATOR\nperiodicity %.3lf [ms]", oscillator->periodicity_ms);
}

static void obj__describe_long_time(obj_t self, char* buffer, int buffer_size) {
    (void) self;
    snprintf(buffer, buffer_size, "TIME");
}

static void obj__describe_long_process(obj_t self, char* buffer, int buffer_size) {
    obj_process_t obj_process = (obj_process_t) self;
    snprintf(
        buffer, buffer_size,
        "PROCESS"
        "\npid: %d"
        "\nexpected status code: %d"
        "\nFDs:"
        "\n  out: r %d w %d"
        "\n  err: r %d w %d",
        obj_process->pid,
        obj_process->success_status_code,
        obj_process->pid_pipe_stdout[0], obj_process->pid_pipe_stdout[1],
        obj_process->pid_pipe_stderr[0], obj_process->pid_pipe_stderr[1]
    );
}

static void obj__describe_long_list(obj_t self, char* buffer, int buffer_size) {
    assert(0 && "lists should be used as temporary containers to create programs with");
    (void) self;
    (void) buffer;
    (void) buffer_size;
}

static void obj__destroy_exec(obj_t self) {
    obj_exec_t exec = (obj_exec_t) self;
    if (exec->cmd_line) {
        free(exec->cmd_line);
        exec->cmd_line = 0;
    }
}

static void obj__destroy_file_modified(obj_t self) {
    (void) self;
}

static void obj__destroy_oscillator(obj_t self) {
    (void) self;
}

static void obj__destroy_time(obj_t self) {
    (void) self;
}

static void obj__destroy_process(obj_t self) {
    obj_process_t obj_process = (obj_process_t) self;

    if (0 < obj_process->pid) {
        if (kill(obj_process->pid, SIGTERM)) {
            perror("kill");
        }
        obj_process->pid = 0;
    }
    
    if (obj_process->pid_pipe_stderr[0]) {
        close(obj_process->pid_pipe_stderr[0]);
        obj_process->pid_pipe_stderr[0] = 0;
    }
    if (obj_process->pid_pipe_stderr[1]) {
        close(obj_process->pid_pipe_stderr[1]);
        obj_process->pid_pipe_stderr[1] = 0;
    }
    if (obj_process->pid_pipe_stdout[0]) {
        close(obj_process->pid_pipe_stdout[0]);
        obj_process->pid_pipe_stdout[0] = 0;
    }
    if (obj_process->pid_pipe_stdout[1]) {
        close(obj_process->pid_pipe_stdout[1]);
        obj_process->pid_pipe_stdout[1] = 0;
    }
}

static void obj__destroy_list(obj_t self) {
    (void) self;
}

int builder__init() {
    if (shared__init(1024 * 64)) {
        return 1;
    }

    _ = shared__calloc(sizeof(*_));
    if (!_) {
        shared__deinit();
        return 1;
    }

    struct timespec t;
    clock_getres(CLOCK_REALTIME, &t);
    _->g_tick_resolution = (double) t.tv_sec + t.tv_nsec / 1000000000.0;
    _->g_time_init = builder__get_time_stamp();

    _->engine_time      = obj__time();
    _->oscillator_200ms = obj__oscillator(_->engine_time, 200);
    // oscillator_10s   = obj__oscillator(engine_time, 10000);
    // c_compiler       = obj__file_modified(oscillator_10s, "/usr/bin/gcc");

    // builder_h = obj__file_modified(oscillator_200ms, "builder.h");
    // builder_c = obj__file_modified(oscillator_200ms, "builder.c");
    // builder_o = obj__file_modified(
    //     obj__list(
    //         obj__process(
    //             obj__exec(
    //                 obj__list(c_compiler, builder_h, builder_c, 0),
    //                 "%s -g -I. -c %s -o builder.o -Wall -Wextra -Werror", obj__file_modified_path(c_compiler), obj__file_modified_path(builder_c)
    //             ),
    //             0,
    //             oscillator_200ms
    //         ),
    //         oscillator_200ms,
    //         0
    //     ),
    //     "builder.o"
    // );

    return 0;
}

void builder__deinit() {
    // not necessary to free shared
    shared__deinit();
}

double builder__get_time_stamp() {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (t.tv_sec * 1000000000 + t.tv_nsec) * _->g_tick_resolution;
}

double builder__get_time_stamp_init() {
    return _->g_time_init;
}

void builder__lock() {
    share__locked();
    share__unlock();
}

void builder__unlock() {
}

obj_t obj__alloc(size_t size) {
    shared__lock();

    obj_t result = obj__alloc_locked(size);
 
    shared__unlock();

    return result;
}

obj_t obj__alloc_locked(size_t size) {
   obj_t result = shared__calloc_locked(size);

    if (_->objects_size <= _->objects_top) {
        if (_->objects_size == 0) {
            _->objects_size = 8;
            _->objects = shared__calloc_locked(_->objects_size * sizeof(*_->objects));
        } else {
            _->objects_size <<= 1;
            _->objects = shared__realloc_locked(_->objects, _->objects_size * sizeof(*_->objects));
        }
    }
    _->objects[_->objects_top++] = result;

    // if (builder_o) {
    //     obj__push_input((obj_t) result, builder_o);
    // }

    return result;
}

void obj__destroy(obj_t self) {
    shared__lock();

    for (size_t object_index = 0; object_index < objects_top; ++object_index) {
        if (objects[object_index] == self) {
            while (object_index < objects_top - 1) {
                objects[object_index] = objects[object_index + 1];
                ++object_index;
            }
            --objects_top;
        }
    }

    self->destroy(self);

    shared__unlock();
}

static void obj__push_input_locked(obj_t self, obj_t input) {
    assert(!(self->transient_flag == 2 && input->transient_flag == 2) && "cant compose lists");

    if (self->transient_flag == 2) {
        assert(self->outputs_top == 0);
        for (size_t input_index = 0; input_index < input->inputs_top; ++input_index) {
            obj_t obj = self->inputs[input_index];
            for (size_t output_index = 0; output_index < obj->outputs_top; ++output_index) {
                obj_t output_obj = obj->outputs[output_index];
                if (output_obj == self) {
                    while (output_index + 1 < obj->outputs_top) {
                        obj->outputs[output_index] = obj->outputs[output_index + 1];
                        ++output_index;
                    }
                    --obj->outputs_top;
                }
            }
            obj__push_input(obj, input);
            self->inputs[input_index] = 0;
        }
        self->inputs_top = 0;
        self->destroy(self);
        return ;
    }

    if (input->transient_flag == 2) {
        assert(input->outputs_top == 0);
        for (size_t input_index = 0; input_index < input->inputs_top; ++input_index) {
            obj_t obj = input->inputs[input_index];
            for (size_t output_index = 0; output_index < obj->outputs_top; ++output_index) {
                obj_t output_obj = obj->outputs[output_index];
                if (output_obj == input) {
                    while (output_index + 1 < obj->outputs_top) {
                        obj->outputs[output_index] = obj->outputs[output_index + 1];
                        ++output_index;
                    }
                    --obj->outputs_top;
                }
            }
            obj__push_input(self, obj);
            input->inputs[input_index] = 0;
        }
        input->inputs_top = 0;
        input->destroy(input);
        return ;
    }

    assert(self != input);
    ARRAY_ENSURE_TOP(self->inputs, self->inputs_top, self->inputs_size);
    self->inputs[self->inputs_top++] = input;
    ARRAY_ENSURE_TOP(input->outputs, input->outputs_top, input->outputs_size);
    input->outputs[input->outputs_top++] = self;

    assert(self->transient_flag == 0);
    obj__check_for_cyclic_input(self);
}

void obj__push_input(obj_t self, obj_t input) {
    shared__lock();
    obj__push_input_locked(self, input);
    shared__unlock();
}

struct attr obj__get_attr(obj_t self) {
    struct attr result;
    pthread_mutex_lock(&self->mutex_attr);
    memcpy(&result, &self->attr, sizeof(self->attr));
    pthread_mutex_unlock(&self->mutex_attr);
    return result;
}

void obj__set_start(obj_t self, double time) {
    pthread_mutex_lock(&self->mutex_attr);
    self->attr.time_start = time;
    self->attr.is_running = 1;
    ++self->attr.n_run;
    pthread_mutex_unlock(&self->mutex_attr);
}

void obj__set_success(obj_t self, double time) {
    pthread_mutex_lock(&self->mutex_attr);
    self->attr.time_success = time;
    ++self->attr.n_success;
    pthread_mutex_unlock(&self->mutex_attr);
}

void obj__set_fail(obj_t self, double time) {
    pthread_mutex_lock(&self->mutex_attr);
    self->attr.time_fail = time;
    ++self->attr.n_fail;
    pthread_mutex_unlock(&self->mutex_attr);
}

void obj__set_finish(obj_t self, double time) {
    pthread_mutex_lock(&self->mutex_attr);
    self->attr.time_finish = time;
    self->attr.is_running = 0;
    pthread_mutex_unlock(&self->mutex_attr);
}

static void obj__run_helper(obj_t self, obj_t caller, int do_input_checks) {
    obj_t obj_proxy = self->obj_proxy;

    struct attr attr = obj__get_attr(self);

    if (obj_proxy && caller == obj_proxy) {
        assert(!attr.is_running);
        assert(attr.time_fail < attr.time_success);
        struct attr attr = obj__get_attr(self);

        if (attr.is_running || attr.time_success <= attr.time_fail) {
            return ;
        }

        for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
            obj_t output = self->outputs[output_index];
            struct attr attr_output = obj__get_attr(output);
            if (attr_output.time_start < attr.time_success) {
                obj__run_helper(output, self, 1);
            }
        }
        return ;
    }
    
    if (do_input_checks && 0 < self->inputs_top) {
        double time_most_successful_input = 0.0;
        for (size_t input_index = 0; input_index < self->inputs_top; ++input_index) {
            obj_t input = self->inputs[input_index];
            if (input == obj_proxy) {
                continue ;
            }
            struct attr attr_input = obj__get_attr(input);
            if (attr_input.time_success < attr_input.time_fail) {
                return ;
            }
            time_most_successful_input = fmax(time_most_successful_input, attr_input.time_success);
        }

        if (time_most_successful_input <= attr.time_start) {
            return ;
        }

        // uncomment to automatically run root programs
        if (0 < self->outputs_top) {
            double time_least_successful_output = INFINITY;
            for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
                obj_t output = self->outputs[output_index];
                struct attr attr_output = obj__get_attr(output);
                time_least_successful_output = fmin(time_least_successful_output, attr_output.time_success);
            }
            if (time_most_successful_input <= time_least_successful_output) {
                return ;
            }
        }
    }

    if (obj_proxy) {
        obj__start_proxy(obj_proxy);
    } else {
        self->run(self);

        struct attr attr = obj__get_attr(self);
        if (attr.is_running || attr.time_success <= attr.time_fail) {
            return ;
        }

        for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
            obj_t output = self->outputs[output_index];
            struct attr attr_output = obj__get_attr(output);
            if (attr_output.time_start < attr.time_success) {
                obj__run_helper(output, self, 1);
            }
        }
    }
}

int obj__run(obj_t self) {
    obj__run_helper(self, 0, 0);
    struct attr attr = obj__get_attr(self);
    return attr.is_running || attr.time_success <= attr.time_fail;
}

void obj__describe_short(obj_t self, char* buffer, int buffer_size) {
    self->describe_short(self, buffer, buffer_size);
}

void obj__describe_long(obj_t self, char* buffer, int buffer_size) {
    char self_describe_long[512];
    self->describe_long(self, self_describe_long, sizeof(self_describe_long));

    struct attr attr = obj__get_attr(self);

    char run_status[32];
    if (attr.is_running) {
        snprintf(run_status, sizeof(run_status), "Running");
    } else {
        if (attr.time_success < attr.time_fail) {
            snprintf(run_status, sizeof(run_status), "Failed");
        } else if (attr.time_fail < attr.time_success) {
            snprintf(run_status, sizeof(run_status), "Succeeded");
        } else if (attr.time_start == 0.0) {
            snprintf(run_status, sizeof(run_status), "Never ran");
        } else {
            snprintf(run_status, sizeof(run_status), "Ran, but no result");
        }
    }
    char abs_time_run_success[128];
    char rel_time_run_success[128];
    if (attr.time_success == 0) {
        snprintf(abs_time_run_success, sizeof(abs_time_run_success), "-");
        snprintf(rel_time_run_success, sizeof(rel_time_run_success), "-");
    } else {
        time_t time_ran_successfully_time_t = (time_t) attr.time_success;
        struct tm* t = localtime(&time_ran_successfully_time_t);
        snprintf(abs_time_run_success, sizeof(abs_time_run_success), "%02d/%02d/%d %02d:%02d:%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
        snprintf(rel_time_run_success, sizeof(rel_time_run_success), "%.2lf", attr.time_success - builder__get_time_stamp_init());
    }
    char abs_time_run_fail[128];
    char rel_time_run_fail[128];
    if (attr.time_fail == 0) {
        snprintf(abs_time_run_fail, sizeof(abs_time_run_fail), "-");
        snprintf(rel_time_run_fail, sizeof(rel_time_run_fail), "-");
    } else {
        time_t time_ran_successfully_time_t = (time_t) attr.time_fail;
        struct tm* t = localtime(&time_ran_successfully_time_t);
        snprintf(abs_time_run_fail, sizeof(abs_time_run_fail), "%02d/%02d/%d %02d:%02d:%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
        snprintf(rel_time_run_fail, sizeof(rel_time_run_fail), "%.2lf", attr.time_fail - builder__get_time_stamp_init());
    }
    char abs_time_run_start[128];
    char rel_time_run_start[128];
    if (attr.time_start == 0) {
        snprintf(abs_time_run_start, sizeof(abs_time_run_start), "-");
        snprintf(rel_time_run_start, sizeof(rel_time_run_start), "-");
    } else {
        time_t time_ran_successfully_time_t = (time_t) attr.time_start;
        struct tm* t = localtime(&time_ran_successfully_time_t);
        snprintf(abs_time_run_start, sizeof(abs_time_run_start), "%02d/%02d/%d %02d:%02d:%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
        snprintf(rel_time_run_start, sizeof(rel_time_run_start), "%.2lf", attr.time_start - builder__get_time_stamp_init());
    }
    char abs_time_run_finish[128];
    char rel_time_run_finish[128];
    if (attr.time_finish == 0) {
        snprintf(abs_time_run_finish, sizeof(abs_time_run_finish), "-");
        snprintf(rel_time_run_finish, sizeof(rel_time_run_finish), "-");
    } else {
        time_t time_ran_successfully_time_t = (time_t) attr.time_finish;
        struct tm* t = localtime(&time_ran_successfully_time_t);
        snprintf(abs_time_run_finish, sizeof(abs_time_run_finish), "%02d/%02d/%d %02d:%02d:%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
        snprintf(rel_time_run_finish, sizeof(rel_time_run_finish), "%.2lf", attr.time_finish - builder__get_time_stamp_init());
    }

    snprintf(
        buffer, buffer_size,
        "%s"
        "\nStatus: %s"
        "\nAbsolute times:"
        "\n  success: %s"
        "\n  failure: %s"
        "\n  start:   %s"
        "\n  finish:  %s"
        "\nRelative times:"
        "\n  success: %s"
        "\n  failure: %s"
        "\n  start:   %s"
        "\n  finish:  %s"
        "\nTotal runs:      %lu"
        "\nSuccessful runs: %lu"
        "\nFailed runs:     %lu",
        self_describe_long,
        run_status,
        abs_time_run_success,
        abs_time_run_fail,
        abs_time_run_start,
        abs_time_run_finish,
        rel_time_run_success,
        rel_time_run_fail,
        rel_time_run_start,
        rel_time_run_finish,
        attr.n_run,
        attr.n_success,
        attr.n_fail
    );
}

obj_t obj__file_modified(obj_t opt_inputs, const char* path, ...) {
    obj_file_modified_t result = (obj_file_modified_t) obj__alloc(sizeof(*result));

    result->base.run            = &obj__run_file_modified;
    result->base.describe_short = &obj__describe_short_file_modified;
    result->base.describe_long  = &obj__describe_long_file_modified;
    result->base.destroy        = &obj__destroy_file_modified;

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

    obj__run((obj_t) result);

    if (opt_inputs) {
        obj__push_input((obj_t) result, opt_inputs);
    }

    return (obj_t) result;
}

const char* obj__file_modified_path(obj_t self) {
    return ((obj_file_modified_t) self)->path;
}

obj_t obj__exec(obj_t opt_inputs, const char* cmd_line, ...) {
    obj_exec_t result = (obj_exec_t) obj__alloc(sizeof(*result));

    result->base.run            = &obj__run_exec;
    result->base.describe_short = &obj__describe_short_exec;
    result->base.describe_long  = &obj__describe_long_exec;
    result->base.destroy        = &obj__destroy_exec;

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

    if (opt_inputs) {
        obj__push_input((obj_t) result, opt_inputs);
    }

    return (obj_t) result;
}

obj_t obj__oscillator(obj_t input, double periodicity_ms) {
    obj_oscillator_t result = (obj_oscillator_t) obj__alloc(sizeof(*result));

    result->base.run            = &obj__run_oscillator;
    result->base.describe_short = &obj__describe_short_oscillator;
    result->base.describe_long  = &obj__describe_long_oscillator;
    result->base.destroy        = &obj__destroy_oscillator;

    result->periodicity_ms = periodicity_ms;

    obj__push_input((obj_t) result, input);

    return (obj_t) result;
}

obj_t obj__time() {
    obj_time_t result = (obj_time_t) obj__alloc(sizeof(*result));

    result->base.run            = &obj__run_time;
    result->base.describe_short = &obj__describe_short_time;
    result->base.describe_long  = &obj__describe_long_time;
    result->base.destroy        = &obj__destroy_time;

    return (obj_t) result;
}

obj_t obj__thread(obj_t obj_to_thread, obj_t collector) {
    obj_thread_t result = (obj_thread_t) obj__alloc(sizeof(*result));

    assert(!obj_to_thread->obj_proxy && "input already has a proxy");

    result->base.run            = &obj__run_thread;
    result->base.describe_short = &obj__describe_short_thread;
    result->base.describe_long  = &obj__describe_long_thread;
    result->base.destroy        = &obj__destroy_thread;

    result->base.is_thread = 1;

    obj__push_input((obj_t) result, collector);
    obj__push_input(obj_to_thread, (obj_t) result);

    obj_to_thread->obj_proxy = (obj_t) result;

    return obj_to_thread;
}

obj_t obj__process(obj_t obj_to_create_process_for, int success_status_code, obj_t collector) {
    obj_process_t result = (obj_process_t) obj__alloc(sizeof(*result));

    assert(!obj_to_create_process_for->obj_proxy && "input already has a process");

    result->base.run            = &obj__run_process;
    result->base.describe_short = &obj__describe_short_process;
    result->base.describe_long  = &obj__describe_long_process;
    result->base.destroy        = &obj__destroy_process;

    result->base.is_process = 1;

    result->success_status_code = success_status_code;

    obj__push_input((obj_t) result, collector);
    obj__push_input(obj_to_create_process_for, (obj_t) result);

    obj_to_create_process_for->obj_proxy = (obj_t) result;

    result->process = process__create();
    if (!result->process) {
        obj__destroy((obj_t) result);
        return 0;
    }

    return obj_to_create_process_for;
}

obj_t obj__list(obj_t obj0, ... /*, 0*/) {
    obj_list_t result = (obj_list_t) obj__alloc(sizeof(*result));

    result->base.run            = &obj__run_list;
    result->base.describe_short = &obj__describe_short_list;
    result->base.describe_long  = &obj__describe_long_list;
    result->base.destroy        = &obj__destroy_list;

    obj_t obj = obj0;
    assert(obj->transient_flag != 2 && "cant compose lists");
    obj__push_input((obj_t) result, obj0);

    va_list ap;
    va_start(ap, obj0);

    obj = va_arg(ap, obj_t);
    while (obj) {
        assert(obj->transient_flag != 2 && "cant compose lists");
        obj__push_input((obj_t) result, obj);
        obj = va_arg(ap, obj_t);
    }
    va_end(ap);

    // Should be unpacked and deleted when taken as input/output
    result->base.transient_flag = 2;

    return (obj_t) result;
}
