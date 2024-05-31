#include "builder.h"

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
#define ARRAY_ENSURE_TOP(array_p, array_top, array_size) do { \
    if ((array_top) >= (array_size)) { \
        if ((array_size) == 0) { \
            (array_size) = 8; \
            (array_p) = malloc((array_size) * sizeof(*(array_p))); \
        } else { \
            (array_size) <<= 1; \
            (array_p) = realloc((array_p), (array_size) * sizeof(*(array_p))); \
            } \
    } \
} while (0)

static double          g_tick_resolution;
static double          g_time_init;
static pthread_mutex_t g_mutex_print;

static size_t async_obj_top;
static obj_t  async_obj[256];
static void   async_obj__signal_handler(int signal);
static void   async_obj__add(obj_t self);
static void   async_obj__remove(obj_t self);

typedef struct obj_file_modified {
    struct obj base;
    char*      path;
} *obj_file_modified_t;

typedef struct obj_sh {
    struct obj base;
    char*      cmd_line;
    pid_t      pid;
    int        success_status_code;
    int        pid_pipe_stdout[2];
    int        pid_pipe_stderr[2];
} *obj_sh_t;

typedef struct obj_oscillator {
    struct obj base;
    double     periodicity_ms;
    double     time_ran_mod;
} *obj_oscillator_t;

typedef struct obj_time {
    struct obj base;
} *obj_time_t;

typedef struct obj_thread {
    struct obj base;
} *obj_thread_t;

typedef struct obj_list {
    struct obj base;
} *obj_list_t;

static int  _prefix_lines(char* dst, int dst_size, const char* src, const char* prefix);

static obj_t  obj__alloc(size_t size);
static void   obj__destroy(obj_t self);
static void   obj__push_input(obj_t self, obj_t input);
static void   obj__check_for_cyclic_input(obj_t self);
static void   obj__set_start(obj_t self, double time);
static void   obj__set_success(obj_t self, double time);
static void   obj__set_fail(obj_t self, double time);
static void   obj__set_finish(obj_t self, double time);
static void   obj__print(obj_t self, const char* format, ...);

static void obj__run_sh(obj_t self);
static void obj__run_file_modified(obj_t self);
static void obj__run_oscillator(obj_t self);
static void obj__run_time(obj_t self);
static void obj__run_thread(obj_t self);
static void obj__run_list(obj_t self);

static void obj__describe_short_sh(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_file_modified(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_oscillator(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_time(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_thread(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_list(obj_t self, char* buffer, int buffer_size);

static void obj__describe_long_sh(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_file_modified(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_oscillator(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_time(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_thread(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_list(obj_t self, char* buffer, int buffer_size);

static void obj__destroy_sh(obj_t self);
static void obj__destroy_file_modified(obj_t self);
static void obj__destroy_oscillator(obj_t self);
static void obj__destroy_time(obj_t self);
static void obj__destroy_thread(obj_t self);
static void obj__destroy_list(obj_t self);

static obj_t obj__alloc(size_t size) {
    obj_t result = calloc(1, size);

    if (pthread_mutex_init(&result->mutex_attr, 0)) {
        perror("pthread_mutex_init");
        free(result);
        return 0;
    }

    return result;
}

static void obj__destroy(obj_t self) {
    self->destroy(self);

    if (self->thread_watcher) {
        // todo: implement
        // if (self->attr.is_running) {
        //     // obj__thread_kill(self);
        // }
    }
    pthread_mutex_destroy(&self->mutex_attr);
}

static void obj__push_input(obj_t self, obj_t input) {
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

static int _prefix_lines(char* dst, int dst_size, const char* src, const char* prefix) {
    if (dst_size < 2) {
        return 0;
    }
    dst[0] = '\0';

    size_t src_index = 0;
    int is_line_prefixed = 0;
    int dst_index = 0;
    while (src[src_index]) {
        if (!is_line_prefixed) {
            int bytes_written = snprintf(
                dst + dst_index,
                dst_size - dst_index,
                "%s",
                prefix
            );
            if (bytes_written < 0) {
                dprintf(STDOUT_FILENO, "snprintf failed\n");
                kill(getpid(), SIGTERM);
                exit(1);
            }
            dst_index += bytes_written;
            is_line_prefixed = 1;
        }
        if (src[src_index] == '\n') {
            is_line_prefixed = 0;
        }
        
        if (dst_size <= dst_index) {
            break ;
        }
        dst[dst_index++] = src[src_index++];
    }

    if (dst_size < dst_index + 2) {
        dst_index = dst_size - 2;
    }
    dst[dst_index] = '\n';
    dst[dst_index + 1] = '\0';


    return dst_index - 1;
}

static void obj__set_start(obj_t self, double time) {
    pthread_mutex_lock(&self->mutex_attr);
    self->attr.time_start = time;
    self->attr.is_running = 1;
    ++self->attr.n_run;
    async_obj__add(self);
    pthread_mutex_unlock(&self->mutex_attr);
}

static void obj__set_success(obj_t self, double time) {
    pthread_mutex_lock(&self->mutex_attr);
    self->attr.time_success = time;
    ++self->attr.n_success;
    pthread_mutex_unlock(&self->mutex_attr);
}

static void obj__set_fail(obj_t self, double time) {
    pthread_mutex_lock(&self->mutex_attr);
    self->attr.time_fail = time;
    ++self->attr.n_fail;
    pthread_mutex_unlock(&self->mutex_attr);
}

static void obj__set_finish(obj_t self, double time) {
    pthread_mutex_lock(&self->mutex_attr);
    self->attr.time_finish = time;
    self->attr.is_running = 0;
    async_obj__remove(self);
    pthread_mutex_unlock(&self->mutex_attr);
}

static void obj__print(obj_t self, const char* format, ...) {
    static char buffer[4096];
    static char buffer2[4096];

    pthread_mutex_lock(&g_mutex_print);

    self->describe_short(self, buffer, sizeof(buffer));
    printf("%.3fs [%d] %s\n", builder__get_time_stamp() - g_time_init, gettid(), buffer);

    va_list ap;
    va_start(ap, format);
    int read_bytes = vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);
    if (read_bytes == -1) {
        dprintf(STDOUT_FILENO, "vsnprintf failed");
        pthread_mutex_unlock(&g_mutex_print);
        kill(getpid(), SIGTERM);
        return ;
    }

    if (0 < read_bytes && 0 < _prefix_lines(buffer2, sizeof(buffer2), buffer, "    ")) {
        printf("%s", buffer2);
    }

    pthread_mutex_unlock(&g_mutex_print);
}

static void obj__run_sh(obj_t self) {
    obj__set_start(self, builder__get_time_stamp());

    obj_sh_t sh = (obj_sh_t) self;

    if (0 < sh->pid) {
        if (kill(sh->pid, SIGTERM)) {
            perror("kill");
        }
    }

    sh->pid = fork();

    if (sh->pid < 0) {
        perror(0);
        exit(1);
    }
    if (sh->pid == 0) {
        close(sh->pid_pipe_stdout[0]);
        if (dup2(sh->pid_pipe_stdout[1], STDOUT_FILENO) == -1) {
            perror(0);
            exit(1);
        }
        close(sh->pid_pipe_stdout[1]);
        close(sh->pid_pipe_stderr[0]);
        if (dup2(sh->pid_pipe_stderr[1], STDERR_FILENO) == -1) {
            perror(0);
            exit(1);
        }
        close(sh->pid_pipe_stderr[1]);
        execl("/usr/bin/sh", "sh", "-c", sh->cmd_line, NULL);
        perror(0);
        exit(1);
    }

    char buffer[4096];
    char buffer2[4096];
    obj__print(self, "");

    int process_finished = 0;
    while (!process_finished) {
        usleep(200000);

        int wstatus = -1;
        pid_t waited_pid = waitpid(sh->pid, &wstatus, WNOHANG);

        ssize_t read_bytes = read(sh->pid_pipe_stdout[0], buffer, sizeof(buffer) - 1);
        if (0 < read_bytes) {
            buffer[read_bytes] = '\0';
            if (0 < _prefix_lines(buffer2, sizeof(buffer2), buffer, "    ")) {
                obj__print(self, buffer2);
            }
        }
        read_bytes = read(sh->pid_pipe_stderr[0], buffer, sizeof(buffer) - 1);
        if (0 < read_bytes) {
            buffer[read_bytes] = '\0';
            if (0 < _prefix_lines(buffer2, sizeof(buffer2), buffer, "    ")) {
                obj__print(self, buffer2);
            }
        }
        
        if (waited_pid == -1) {
            process_finished = 1;
            obj__set_fail(self, builder__get_time_stamp());
        } else if (waited_pid > 0) {
            assert(waited_pid == sh->pid);
            if (WIFEXITED(wstatus)) {
                wstatus = WEXITSTATUS(wstatus);
                if (sh->success_status_code == wstatus) {
                    process_finished = 1;
                    obj__set_success(self, builder__get_time_stamp());
                    obj__print(self, "finished successfully with status code: %d", wstatus);
                } else {
                    process_finished = 1;
                    obj__set_fail(self, builder__get_time_stamp());
                    obj__print(self, "finished with failure status code: %d", wstatus);
                }
            } else if (WIFSIGNALED(wstatus)) {
                int sig = WTERMSIG(wstatus);
                obj__print(self, "was terminated by signal: %d %s", sig, strsignal(sig));
            } else if (WIFSTOPPED(wstatus)) {
                int sig = WSTOPSIG(wstatus);
                obj__print(self, "was stopped by signal: %d %s", sig, strsignal(sig));
            } else if (WIFCONTINUED(wstatus)) {
                obj__print(self, "was continued by signal %d %s", SIGCONT, strsignal(SIGCONT));
            } else {
                assert(0);
            }
            process_finished = 1;
        }
    }

    ssize_t read_bytes = read(sh->pid_pipe_stdout[0], buffer, sizeof(buffer) - 1);
    if (0 < read_bytes) {
        buffer[read_bytes] = '\0';
        if (0 < _prefix_lines(buffer2, sizeof(buffer2), buffer, "    ")) {
            obj__print(self, buffer2);
        }
    }
    read_bytes = read(sh->pid_pipe_stderr[0], buffer, sizeof(buffer) - 1);
    if (0 < read_bytes) {
        buffer[read_bytes] = '\0';
        if (0 < _prefix_lines(buffer2, sizeof(buffer2), buffer, "    ")) {
            obj__print(self, buffer2);
        }
    }

    sh->pid = 0;

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

static void* thread_worker_routine(void* data) {
    obj_t obj = (obj_t) data;

    obj->run(obj);

    return data;
}

static void obj__run_thread(obj_t self) {
    struct attr attr = obj__get_attr(self);
    if (!attr.is_running) {
        return ;
    }

    const double time_cur = builder__get_time_stamp();

    int found_running_output = 0;
    int found_non_successful_output = 0;
    for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
        obj_t output = (obj_t) self->outputs[output_index];
        struct attr attr_output_prev = obj__get_attr(output);
        if (attr_output_prev.is_running) {
            int tryjoin_result = pthread_tryjoin_np(output->thread, 0);
            if (!tryjoin_result) {
                struct attr attr_output_cur = obj__get_attr(output);
                if (attr_output_cur.time_success <= attr_output_cur.time_fail) {
                    found_non_successful_output = 1;
                }
                obj__set_finish(output, time_cur);
            } else if (tryjoin_result == EBUSY) {
                found_running_output = 1;
            } else {
                perror("pthread_tryjoin_np");
                kill(getpid(), SIGTERM);
            }
        } else {
            if (attr_output_prev.time_success <= attr_output_prev.time_fail) {
                found_non_successful_output = 1;
            }
        }
    }

    if (!found_running_output) {
        if (found_non_successful_output) {
            obj__set_fail(self, time_cur);
        } else {
            obj__set_success(self, time_cur);
        }
        obj__set_finish(self, time_cur);
    }
}

static void obj__run_list(obj_t self) {
    (void) self;
    assert(0 && "lists should be used as temporary containers to create programs with");
}

static void obj__describe_short_sh(obj_t self, char* buffer, int buffer_size) {
    obj_sh_t sh = (obj_sh_t) self;
    snprintf(
        buffer, buffer_size,
        "/usr/bin/sh -c \"%s\", pid: %d, expected status code: %d",
        sh->cmd_line,
        sh->pid,
        sh->success_status_code
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

static void obj__describe_short_thread(obj_t self, char* buffer, int buffer_size) {
    (void) self;
    snprintf(buffer, buffer_size, "THREAD");
}

static void obj__describe_short_list(obj_t self, char* buffer, int buffer_size) {
    assert(0 && "lists should be used as temporary containers to create programs with");
    (void) self;
    (void) buffer;
    (void) buffer_size;
}

static void obj__describe_long_sh(obj_t self, char* buffer, int buffer_size) {
    obj_sh_t sh = (obj_sh_t) self;
    snprintf(buffer, buffer_size,
        "SH"
        "\nPid:%d"
        "\nFDs:"
        "\n stdout -> r %d  w %d"
        "\n stderr -> r %d  w %d"
        "\n%s"
        "\nExpected status code: %d",
        sh->pid,
        sh->pid_pipe_stdout[0], sh->pid_pipe_stdout[1],
        sh->pid_pipe_stderr[0], sh->pid_pipe_stderr[1],
        sh->cmd_line,
        sh->success_status_code
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

static void obj__describe_long_thread(obj_t self, char* buffer, int buffer_size) {
    (void) self;
    snprintf(buffer, buffer_size, "THREAD");
}

static void obj__describe_long_list(obj_t self, char* buffer, int buffer_size) {
    assert(0 && "lists should be used as temporary containers to create programs with");
    (void) self;
    (void) buffer;
    (void) buffer_size;
}

static void obj__destroy_sh(obj_t self) {
    obj_sh_t sh = (obj_sh_t) self;
    if (sh->cmd_line) {
        free(sh->cmd_line);
        sh->cmd_line = 0;
    }
    if (sh->pid_pipe_stderr[0]) {
        close(sh->pid_pipe_stderr[0]);
        sh->pid_pipe_stderr[0] = 0;
    }
    if (sh->pid_pipe_stderr[1]) {
        close(sh->pid_pipe_stderr[1]);
        sh->pid_pipe_stderr[1] = 0;
    }
    if (sh->pid_pipe_stdout[0]) {
        close(sh->pid_pipe_stdout[0]);
        sh->pid_pipe_stdout[0] = 0;
    }
    if (sh->pid_pipe_stdout[1]) {
        close(sh->pid_pipe_stdout[1]);
        sh->pid_pipe_stdout[1] = 0;
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

static void obj__destroy_thread(obj_t self) {
    (void) self;
}

static void obj__destroy_list(obj_t self) {
    (void) self;
}

static void async_obj__signal_handler(int signal) {
    if (signal == SIGTERM) {
        for (size_t i = 0; i < ARRAY_SIZE(async_obj); ++i) {
            obj_t obj = async_obj[i];
            if (obj) {
                obj__destroy(obj);
            }
        }
        exit(1);
    } else {
        printf("%d %s\n", signal, strsignal(signal));
        assert(0 && "Signal is unhandled.");
    }
}

static void async_obj__add(obj_t self) {
    for (size_t i = 0; i < ARRAY_SIZE(async_obj); ++i) {
        if (async_obj[i] == 0) {
            async_obj[i] = self;
            assert(async_obj_top++ < ARRAY_SIZE(async_obj));
            break ;
        }
    }
}

static void async_obj__remove(obj_t self) {
    for (size_t i = 0; i < ARRAY_SIZE(async_obj); ++i) {
        if (async_obj[i] == self) {
            async_obj[i] = 0;
            assert(async_obj_top-- > 0);
            break ;
        }
    }
}

int builder__init() {
    struct sigaction act = { 0 };
    act.sa_handler = &async_obj__signal_handler;
    sigaction(SIGTERM, &act, 0);

    struct timespec t;
    clock_getres(CLOCK_REALTIME, &t);
    g_tick_resolution = (double) t.tv_sec + t.tv_nsec / 1000000000.0;

    g_time_init = builder__get_time_stamp();

    if (pthread_mutex_init(&g_mutex_print, 0)) {
        return 1;
    }

    return 0;
}

void builder__deinit() {
    pthread_mutex_destroy(&g_mutex_print);
}

double builder__get_time_stamp() {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (t.tv_sec * 1000000000 + t.tv_nsec) * g_tick_resolution;
}

double builder__get_time_stamp_init() {
    return g_time_init;
}

struct attr obj__get_attr(obj_t self) {
    struct attr result;
    pthread_mutex_lock(&self->mutex_attr);
    memcpy(&result, &self->attr, sizeof(self->attr));
    pthread_mutex_unlock(&self->mutex_attr);
    return result;
}

static void obj__run_helper(obj_t self, obj_t caller) {
    obj_t thread_watcher = self->thread_watcher;

    struct attr attr_prev = obj__get_attr(self);

    if (thread_watcher && caller == thread_watcher) {
        assert(!attr_prev.is_running);
        assert(attr_prev.time_fail < attr_prev.time_success);
        for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
            obj_t output = self->outputs[output_index];
            struct attr attr_output = obj__get_attr(output);
            if (attr_output.time_start < attr_prev.time_success) {
                obj__run_helper(output, self);
            }
        }
        return ;
    }
    
    if (0 < self->inputs_top) {
        double time_most_successful_input = 0.0;
        for (size_t input_index = 0; input_index < self->inputs_top; ++input_index) {
            obj_t input = self->inputs[input_index];
            if (input == thread_watcher) {
                continue ;
            }
            struct attr attr_input = obj__get_attr(input);
            if (attr_input.time_success < attr_input.time_fail) {
                return ;
            }
            time_most_successful_input = fmax(time_most_successful_input, attr_input.time_success);
        }

        if (time_most_successful_input <= attr_prev.time_start) {
            return ;
        }

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

    if (thread_watcher) {
        const double time_cur = builder__get_time_stamp();
        obj__set_start(thread_watcher, time_cur);
        for (size_t output_index = 0; output_index < thread_watcher->outputs_top; ++output_index) {
            obj_t output = thread_watcher->outputs[output_index];
            struct attr attr_output_prev = obj__get_attr(output);
            if (attr_output_prev.is_running) {
                if (pthread_cancel(output->thread)) {
                    perror("pthread_cancel");
                }
                void* thread_result = 0;
                int join_result = pthread_join(output->thread, &thread_result);
                if (join_result) {
                    perror("pthread_join");
                } else {
                    if (thread_result == PTHREAD_CANCELED) {
                        // obj__set_fail(output, builder__get_time_stamp());
                    } else {
                        assert(thread_result == output);
                    }
                }
                obj__set_finish(output, time_cur);
            }
            struct attr attr_output_cur = obj__get_attr(output);
            assert(!attr_output_cur.is_running);
            obj__set_start(output, time_cur);
            if (pthread_create(&output->thread, 0, thread_worker_routine, output)) {
                perror("pthread_create");
                kill(getpid(), SIGTERM);
                exit(1);
            }
        }
    } else {
        self->run(self);

        struct attr attr__cur = obj__get_attr(self);

        if (
            attr__cur.is_running ||
            attr__cur.time_success <= attr__cur.time_fail
        ) {
            return ;
        }

        for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
            obj_t output = self->outputs[output_index];
            struct attr attr_output = obj__get_attr(output);
            if (attr_output.time_start < attr__cur.time_success) {
                obj__run_helper(output, self);
            }
        }
    }
}

int obj__run(obj_t self) {
    obj__run_helper(self, 0);
    struct attr attr = obj__get_attr(self);
    return attr.is_running;
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

obj_t obj__sh(obj_t opt_inputs, int success_status_code, const char* cmd_line, ...) {
    obj_sh_t result = (obj_sh_t) obj__alloc(sizeof(*result));

    result->base.run            = &obj__run_sh;
    result->base.describe_short = &obj__describe_short_sh;
    result->base.describe_long  = &obj__describe_long_sh;
    result->base.destroy        = &obj__destroy_sh;

    result->success_status_code = success_status_code;

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

    if (pipe(result->pid_pipe_stdout) == -1) {
        perror(0);
        obj__destroy((obj_t) result);
        return 0;
    }
    if (pipe(result->pid_pipe_stderr) == -1) {
        perror(0);
        obj__destroy((obj_t) result);
        return 0;
    }
    if (fcntl(result->pid_pipe_stdout[0], F_SETFL, fcntl(result->pid_pipe_stdout[0], F_GETFL) | O_NONBLOCK) == -1) {
        perror(0);
        obj__destroy((obj_t) result);
        return 0;
    }
    if (fcntl(result->pid_pipe_stderr[0], F_SETFL, fcntl(result->pid_pipe_stderr[0], F_GETFL) | O_NONBLOCK) == -1) {
        perror(0);
        obj__destroy((obj_t) result);
        return 0;
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

obj_t obj__thread(obj_t input_to_thread, obj_t collector) {
    obj_thread_t result = (obj_thread_t) obj__alloc(sizeof(*result));

    assert(!input_to_thread->thread_watcher && "input already has a thread");

    result->base.run            = &obj__run_thread;
    result->base.describe_short = &obj__describe_short_thread;
    result->base.describe_long  = &obj__describe_long_thread;
    result->base.destroy        = &obj__destroy_thread;

    result->base.is_thread_watcher = 1;

    obj__push_input((obj_t) result, collector);
    obj__push_input(input_to_thread, (obj_t) result);

    input_to_thread->thread_watcher = (obj_t) result;

    return input_to_thread;
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
