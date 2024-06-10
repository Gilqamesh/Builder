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

struct builder_shared *_;

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
    // any process can set this to indicate to the main process to run the obj
    int        force_start;
} *obj_process_t;

typedef struct obj_list {
    struct obj base;
} *obj_list_t;

static void obj__check_for_cyclic_input(obj_t self);
static void obj__start_proc(obj_t self);

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
        if (self->time_success == 0.0 || 0 < difftime(file_info.st_mtime, (time_t) self->time_success)) {
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

    obj_oscillator_t oscillator = (obj_oscillator_t) self;
    if (self->time_success == 0.0) {
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
    const double delta_time_last_successful_run_s = self->time_success - _->read_only.time_init + epsilon_s;
    const double last_successful_run_mod_s = delta_time_last_successful_run_s - fmod(delta_time_last_successful_run_s, periodicity_s);
    const double delta_time_current_s = builder__get_time_stamp() - _->read_only.time_init;
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

static int obj__process_child_fn(void* data) {
    obj_t self = (obj_t) data;
    obj_process_t obj_process = (obj_process_t) self;

    for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
        obj_t output = self->outputs[output_index];
        output->run(output);
    }

    return obj_process->success_status_code;
}

static void obj__start_proc(obj_t self) {
    const double time_cur = builder__get_time_stamp();

    obj_process_t obj_process = (obj_process_t) self;
    if (obj_process->proc) {
        proc__destroy(obj_process->proc);
        obj__set_fail(self, time_cur);
        obj__set_finish(self, time_cur);
        for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
            obj_t output = self->outputs[output_index];
            if (output->is_running) {
                obj__set_fail(output, time_cur);
                obj__set_finish(output, time_cur);
            }
        }
    }

    obj__set_start(self, time_cur);
    obj_process->proc = proc__create(&obj__process_child_fn, self);
    if (!obj_process->proc) {
        obj__set_fail(self, time_cur);
        obj__set_finish(self, time_cur);
    }
}

static void obj__run_process(obj_t self) {
    shared__lock();

    obj_process_t obj_process = (obj_process_t) self;

    if (!obj_process->proc) {
        shared__unlock();
        return ;
    }

    int wstatus = proc__wait(obj_process->proc, 0);

    // todo: prevent denial-of-service
    char buffer[256];
    while (proc__read(obj_process->proc, buffer, sizeof(buffer))) {
        printf("%s\n", buffer);
    }

    const double time_cur = builder__get_time_stamp();
    if (wstatus == -4) {
        // continued, do nothing
    } else if (wstatus == -3) {
        // stopped, do nothing
    } else if (wstatus == -2) {
        // terminated
        obj__set_fail(self, time_cur);
        obj__set_finish(self, time_cur);
        for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
            obj_t output = self->outputs[output_index];
            if (output->is_running) {
                obj__set_fail(output, time_cur);
                obj__set_finish(output, time_cur);
            }
        }
    } else if (wstatus == -1) {
        // still running, do nothing
    } else {
        assert(0 <= wstatus);
        if (obj_process->success_status_code == wstatus) {
            obj__set_success(self, time_cur);
        } else {
            obj__set_fail(self, time_cur);
        }
        obj__set_finish(self, time_cur);
        proc__destroy(obj_process->proc);
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
    snprintf(buffer, buffer_size, "pid: %d", obj_process->proc->pid);
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
        "\nexpected status code: %d",
        obj_process->proc->pid,
        obj_process->success_status_code
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

    if (obj_process->proc) {
        proc__destroy(obj_process->proc);
        obj_process->proc = 0;
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
    _->read_only.tick_resolution = (double) t.tv_sec + t.tv_nsec / 1000000000.0;
    _->read_only.time_init = builder__get_time_stamp();

    _->engine_time      = obj__time();
    _->oscillator_200ms = obj__oscillator(_->engine_time, 200);
    // _->oscillator_10s   = obj__oscillator(_->engine_time, 10000);
    // _->c_compiler       = obj__file_modified(_->oscillator_10s, "/usr/bin/gcc");

    // _->builder_h = obj__file_modified(_->oscillator_200ms, "builder.h");
    // _->builder_c = obj__file_modified(_->oscillator_200ms, "builder.c");
    // _->builder_o = obj__file_modified(
    //     obj__list(
    //         obj__process(
    //             obj__exec(
    //                 obj__list(_->c_compiler, _->builder_h, _->builder_c, 0),
    //                 "%s -g -I. -c %s -o builder.o -Wall -Wextra -Werror", obj__file_modified_path(_->c_compiler), obj__file_modified_path(_->builder_c)
    //             ),
    //             0,
    //             _->oscillator_200ms
    //         ),
    //         _->oscillator_200ms,
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
    return (t.tv_sec * 1000000000 + t.tv_nsec) * _->read_only.tick_resolution;
}

double builder__get_time_stamp_init() {
    return _->read_only.time_init;
}

void builder__lock() {
    shared__lock();
}

void builder__unlock() {
    shared__unlock();
}

obj_t obj__alloc(size_t size) {
    shared__lock();

    obj_t result = shared__calloc(size);
    assert(result && "handle out of memory");

    if (_->objects_size <= _->objects_top) {
        if (_->objects_size == 0) {
            _->objects_size = 8;
            _->objects = shared__calloc(_->objects_size * sizeof(*_->objects));
        } else {
            _->objects_size <<= 1;
            _->objects = shared__recalloc(_->objects, _->objects_size * sizeof(*_->objects));
        }
    }
    assert(_->objects && "handle out of memory");
    _->objects[_->objects_top++] = result;

    // if (builder_o) {
    //     obj__push_input((obj_t) result, builder_o);
    // }
 
    shared__unlock();

    return result;
}

void obj__destroy(obj_t self) {
    shared__lock();

    assert(0 && "todo: what about programs that are running?");

    for (size_t object_index = 0; object_index < _->objects_top; ++object_index) {
        if (_->objects[object_index] == self) {
            while (object_index < _->objects_top - 1) {
                _->objects[object_index] = _->objects[object_index + 1];
                ++object_index;
            }
            --_->objects_top;
        }
    }

    self->destroy(self);

    shared__unlock();
}

void obj__print(obj_t self, const char* format, ...) {
    shared__lock();

    static char buffer[1024];
    self->describe_short(self, buffer, sizeof(buffer));
    printf("%.3fs [%d] %s\n", builder__get_time_stamp() - _->read_only.time_init, gettid(), buffer);
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);

    shared__unlock();
}

void obj__push_input(obj_t self, obj_t input) {
    shared__lock();

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

        shared__unlock();
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

        shared__unlock();
        return ;
    }

    assert(self->transient_flag == 0);
    assert(self != input);

    if (self->inputs_size <= self->inputs_top) {
        if (self->inputs_size == 0) {
            self->inputs_size = 4;
            self->inputs = shared__malloc(self->inputs_size * sizeof(*self->inputs));
        } else {
            self->inputs_size <<= 1;
            self->inputs = shared__realloc(self->inputs, self->inputs_size * sizeof(*self->inputs));
        }
    }
    assert(self->inputs && "handle out of memory");
    self->inputs[self->inputs_top++] = input;

    if (input->outputs_size <= input->outputs_top) {
        if (input->outputs_size == 0) {
            input->outputs_size = 4;
            input->outputs = shared__malloc(input->outputs_size * sizeof(*input->outputs));
        } else {
            input->outputs_size <<= 1;
            input->outputs = shared__realloc(input->outputs, input->outputs_size * sizeof(*input->outputs));
        }
    }
    assert(input->outputs && "handle out of memory");
    input->outputs[input->outputs_top++] = self;

    obj__check_for_cyclic_input(self);

    shared__unlock();
}

void obj__set_start(obj_t self, double time) {
    shared__lock();

    self->time_start = time;
    self->is_running = 1;
    ++self->n_run;

    shared__unlock();
}

void obj__set_success(obj_t self, double time) {
    shared__lock();

    self->time_success = time;
    ++self->n_success;

    shared__unlock();
}

void obj__set_fail(obj_t self, double time) {
    shared__lock();

    self->time_fail = time;
    ++self->n_fail;

    shared__unlock();
}

void obj__set_finish(obj_t self, double time) {
    shared__lock();

    self->time_finish = time;
    self->is_running = 0;

    shared__unlock();
}

static int obj__check_inputs(obj_t self) {
    if (self->inputs_top == 0) {
        return 1;
    }

    double time_most_successful_input = 0.0;
    for (size_t input_index = 0; input_index < self->inputs_top; ++input_index) {
        obj_t input = self->inputs[input_index];
        if (input == self->obj_proc) {
            continue ;
        }
        if (input->time_success < input->time_fail) {
            return 1;
        }
        time_most_successful_input = fmax(time_most_successful_input, input->time_success);
    }

    if (time_most_successful_input <= self->time_start) {
        return 1;
    }

    // uncomment to automatically run root programs
    if (0 < self->outputs_top) {
        double time_least_successful_output = INFINITY;
        for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
            obj_t output = self->outputs[output_index];
            time_least_successful_output = fmin(time_least_successful_output, output->time_success);
        }
        if (time_most_successful_input <= time_least_successful_output) {
            return 1;
        }
    }
    
    return 0;
}

static void obj__run_helper(obj_t self, obj_t caller, int do_input_checks) {
    shared__lock();

    if (self->is_proc) {
        obj_process_t obj_proc = (obj_process_t) self;
        if (obj_proc->force_start) {
            obj_proc->force_start = 0;
            obj__start_proc(self);
            shared__unlock();
            return ;
        }
    }

    if (self->obj_proc && caller == self->obj_proc) {
        if (self->is_running || self->time_success <= self->time_fail) {
            shared__unlock();
            return ;
        }

        if (do_input_checks && obj__check_inputs(self)) {
            shared__unlock();
            return ;
        }

        for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
            obj_t output = self->outputs[output_index];
            if (output->time_start < self->time_success) {
                obj__run_helper(output, self, 1);
            }
        }
        shared__unlock();
        return ;
    }
    
    if (
        do_input_checks &&
        obj__check_inputs(self)
    ) {
        shared__unlock();
        return ;
    }

    if (self->obj_proc) {
        obj__start_proc(self->obj_proc);
    } else {
        self->run(self);

        if (self->is_running || self->time_success <= self->time_fail) {
            shared__unlock();
            return ;
        }

        for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
            obj_t output = self->outputs[output_index];
            if (output->time_start < self->time_success) {
                obj__run_helper(output, self, 1);
            }
        }
    }

    shared__unlock();
}

int obj__run(obj_t self) {
    shared__lock();

    obj_process_t obj_proc = 0;
    if (self->is_proc) {
        obj_proc = (obj_process_t) self;
    } else if (self->obj_proc) {
        obj_proc = (obj_process_t) self->obj_proc;
    }

    if (
        obj_proc &&
        obj_proc->proc &&
        getpid() == obj_proc->proc->pid
    ) {
        obj_proc->force_start = 1;
        shared__unlock();
        return 1;
    }

    obj__run_helper(self, 0, 0);
    int result = self->is_running || self->time_success <= self->time_fail;

    shared__unlock();

    return result;
}

void obj__describe_short(obj_t self, char* buffer, int buffer_size) {
    shared__lock();

    self->describe_short(self, buffer, buffer_size);

    shared__unlock();
}

void obj__describe_long(obj_t self, char* buffer, int buffer_size) {
    shared__lock();

    char self_describe_long[512];
    self->describe_long(self, self_describe_long, sizeof(self_describe_long));

    char run_status[32];
    if (self->is_running) {
        snprintf(run_status, sizeof(run_status), "Running");
    } else {
        if (self->time_success < self->time_fail) {
            snprintf(run_status, sizeof(run_status), "Failed");
        } else if (self->time_fail < self->time_success) {
            snprintf(run_status, sizeof(run_status), "Succeeded");
        } else if (self->time_start == 0.0) {
            snprintf(run_status, sizeof(run_status), "Never ran");
        } else {
            snprintf(run_status, sizeof(run_status), "Ran, but no result");
        }
    }
    char abs_time_run_success[128];
    char rel_time_run_success[128];
    if (self->time_success == 0) {
        snprintf(abs_time_run_success, sizeof(abs_time_run_success), "-");
        snprintf(rel_time_run_success, sizeof(rel_time_run_success), "-");
    } else {
        time_t time_ran_successfully_time_t = (time_t) self->time_success;
        struct tm* t = localtime(&time_ran_successfully_time_t);
        snprintf(abs_time_run_success, sizeof(abs_time_run_success), "%02d/%02d/%d %02d:%02d:%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
        snprintf(rel_time_run_success, sizeof(rel_time_run_success), "%.2lf", self->time_success - builder__get_time_stamp_init());
    }
    char abs_time_run_fail[128];
    char rel_time_run_fail[128];
    if (self->time_fail == 0) {
        snprintf(abs_time_run_fail, sizeof(abs_time_run_fail), "-");
        snprintf(rel_time_run_fail, sizeof(rel_time_run_fail), "-");
    } else {
        time_t time_ran_successfully_time_t = (time_t) self->time_fail;
        struct tm* t = localtime(&time_ran_successfully_time_t);
        snprintf(abs_time_run_fail, sizeof(abs_time_run_fail), "%02d/%02d/%d %02d:%02d:%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
        snprintf(rel_time_run_fail, sizeof(rel_time_run_fail), "%.2lf", self->time_fail - builder__get_time_stamp_init());
    }
    char abs_time_run_start[128];
    char rel_time_run_start[128];
    if (self->time_start == 0) {
        snprintf(abs_time_run_start, sizeof(abs_time_run_start), "-");
        snprintf(rel_time_run_start, sizeof(rel_time_run_start), "-");
    } else {
        time_t time_ran_successfully_time_t = (time_t) self->time_start;
        struct tm* t = localtime(&time_ran_successfully_time_t);
        snprintf(abs_time_run_start, sizeof(abs_time_run_start), "%02d/%02d/%d %02d:%02d:%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
        snprintf(rel_time_run_start, sizeof(rel_time_run_start), "%.2lf", self->time_start - builder__get_time_stamp_init());
    }
    char abs_time_run_finish[128];
    char rel_time_run_finish[128];
    if (self->time_finish == 0) {
        snprintf(abs_time_run_finish, sizeof(abs_time_run_finish), "-");
        snprintf(rel_time_run_finish, sizeof(rel_time_run_finish), "-");
    } else {
        time_t time_ran_successfully_time_t = (time_t) self->time_finish;
        struct tm* t = localtime(&time_ran_successfully_time_t);
        snprintf(abs_time_run_finish, sizeof(abs_time_run_finish), "%02d/%02d/%d %02d:%02d:%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
        snprintf(rel_time_run_finish, sizeof(rel_time_run_finish), "%.2lf", self->time_finish - builder__get_time_stamp_init());
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
        self->n_run,
        self->n_success,
        self->n_fail
    );

    shared__unlock();
}

obj_t obj__file_modified(obj_t opt_inputs, const char* path, ...) {
    shared__lock();

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

    shared__unlock();
}

const char* obj__file_modified_path(obj_t self) {
    shared__lock();

    const char* result = ((obj_file_modified_t) self)->path;

    shared__unlock();

    return result;
}

obj_t obj__exec(obj_t opt_inputs, const char* cmd_line, ...) {
    shared__lock();

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

    shared__unlock();

    return (obj_t) result;
}

obj_t obj__oscillator(obj_t input, double periodicity_ms) {
    shared__lock();

    obj_oscillator_t result = (obj_oscillator_t) obj__alloc(sizeof(*result));

    result->base.run            = &obj__run_oscillator;
    result->base.describe_short = &obj__describe_short_oscillator;
    result->base.describe_long  = &obj__describe_long_oscillator;
    result->base.destroy        = &obj__destroy_oscillator;

    result->periodicity_ms = periodicity_ms;

    obj__push_input((obj_t) result, input);

    shared__unlock();

    return (obj_t) result;
}

obj_t obj__time() {
    shared__lock();

    obj_time_t result = (obj_time_t) obj__alloc(sizeof(*result));

    result->base.run            = &obj__run_time;
    result->base.describe_short = &obj__describe_short_time;
    result->base.describe_long  = &obj__describe_long_time;
    result->base.destroy        = &obj__destroy_time;

    shared__unlock();

    return (obj_t) result;
}

obj_t obj__process(obj_t obj_to_create_process_for, int success_status_code, obj_t collector) {
    shared__lock();

    obj_process_t result = (obj_process_t) obj__alloc(sizeof(*result));

    assert(!obj_to_create_process_for->obj_proc && "input already has a process");

    result->base.run            = &obj__run_process;
    result->base.describe_short = &obj__describe_short_process;
    result->base.describe_long  = &obj__describe_long_process;
    result->base.destroy        = &obj__destroy_process;

    result->base.is_proc = 1;

    result->success_status_code = success_status_code;

    obj__push_input((obj_t) result, collector);
    obj__push_input(obj_to_create_process_for, (obj_t) result);

    obj_to_create_process_for->obj_proc = (obj_t) result;

    shared__unlock();

    return obj_to_create_process_for;
}

obj_t obj__list(obj_t obj0, ... /*, 0*/) {
    shared__lock();

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

    shared__unlock();

    return (obj_t) result;
}
