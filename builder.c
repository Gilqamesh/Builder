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
#include <errno.h>

#include <math.h>

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

static double g_tick_resolution;
static double g_time_init;

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
} *obj_sh_t;

typedef struct obj_oscillator {
    struct obj base;
    double     periodicity_ms;
    double     time_ran_mod;
} *obj_oscillator_t;

typedef struct obj_time {
    struct obj base;
} *obj_time_t;

typedef struct obj_list {
    struct obj base;
} *obj_list_t;

static void obj__push_input(obj_t self, obj_t input);
static void obj__check_for_cyclic_input(obj_t self);
static int  obj__check_for_cyclic_input_helper(obj_t self);
static void obj__print_input_graph_helper(obj_t self, int depth);
static void obj__timed_run(obj_t self, obj_t parent);
static obj_t obj__alloc(size_t size);

static void obj__wait_status_helper(obj_t self, int wstatus);
static void obj__kill(obj_t self);
static void obj__wait(obj_t self);
static void obj__wait_no_hang(obj_t self);

static void obj__run_sh(obj_t self, obj_t parent);
static void obj__run_file_modified(obj_t self, obj_t parent);
static void obj__run_oscillator(obj_t self, obj_t parent);
static void obj__run_time(obj_t self, obj_t parent);
static void obj__run_list(obj_t self, obj_t parent);

static void obj__describe_short_sh(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_file_modified(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_oscillator(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_time(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_list(obj_t self, char* buffer, int buffer_size);

static void obj__describe_long_sh(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_file_modified(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_oscillator(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_time(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_list(obj_t self, char* buffer, int buffer_size);

static void obj__destroy_sh(obj_t self);
static void obj__destroy_file_modified(obj_t self);
static void obj__destroy_oscillator(obj_t self);
static void obj__destroy_time(obj_t self);
static void obj__destroy_list(obj_t self);

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

static void obj__check_for_cyclic_input(obj_t self) {
    assert(self->transient_flag == 0);
    if (obj__check_for_cyclic_input_helper(self)) {
        obj__print_input_graph_helper(self, 0);
        assert(0);
    }
}

static int obj__check_for_cyclic_input_helper(obj_t self) {
    if (self->transient_flag) {
        self->transient_flag = 0;
        printf("Cyclic input detected between:\n");
        return 1;
    }
    self->transient_flag = 1;

    for (size_t input_index = 0; input_index < self->inputs_top; ++input_index) {
        obj_t input = self->inputs[input_index];
        if (obj__check_for_cyclic_input_helper(input)) {
            self->transient_flag = 0;
            char buffer[256];
            self->describe_short(self, buffer, sizeof(buffer));
            printf("%s\n", buffer);
            return 1;
        }
    }

    self->transient_flag = 0;
    
    return 0;
}

static void obj__print_input_graph_helper(obj_t self, int depth) {
    char buffer[256];
    self->describe_short(self, buffer, sizeof(buffer));
    printf("%s\n", buffer);

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

static void obj__describe_time_stamp_prefix(obj_t self, char* buffer, int buffer_size) {
    const double time_since_init = builder__get_time_stamp() - g_time_init;
    snprintf(buffer, buffer_size, "%.2fs [%s%d] ", time_since_init, self->collector ? "~" : "", self->pid);
}

static void obj__wait_status_helper(obj_t self, int wstatus) {
    self->last_run_result = RUN_RESULT_FAILED;

    char prefix[32];
    obj__describe_time_stamp_prefix(self, prefix, sizeof(prefix));
    if (WIFEXITED(wstatus)) {
        wstatus = WEXITSTATUS(wstatus);
        if (self->success_status_code == wstatus) {
            printf("%sfinished successfully with status code: %d\n", prefix, wstatus);
            self->last_run_result = RUN_RESULT_SUCESSS;
        } else {
            printf("%sfinished with failure status code: %d\n", prefix, wstatus);
        }
    } else if (WIFSIGNALED(wstatus)) {
        int sig = WTERMSIG(wstatus);
        printf("%swas terminated by signal: %d %s\n", prefix, sig, strsignal(sig));
    } else if (WIFSTOPPED(wstatus)) {
        int sig = WSTOPSIG(wstatus);
        printf("%swas stopped by signal: %d %s\n", prefix, sig, strsignal(sig));
    } else if (WIFCONTINUED(wstatus)) {
        printf("%swas continued by signal %d %s\n", prefix, SIGCONT, strsignal(SIGCONT));
    } else {
        assert(0);
    }

    self->pid = 0;
    self->is_running = NOT_RUNNING;
}

static void obj__kill(obj_t self) {
    assert(self->pid > 0);
    kill(self->pid, SIGTERM);
    waitpid(self->pid, 0, 0);
    self->pid = 0;
    self->is_running = NOT_RUNNING;
    self->last_run_result = RUN_RESULT_FAILED;
}

static void obj__wait_prefix_message_helper(int fd, char* prefix, int prefix_len) {
    static char buffer[1024];

    ssize_t read_bytes = read(fd, buffer, sizeof(buffer) - 1);
    if (read_bytes == -1) {
        if (errno == EAGAIN) {
            return ;
        } else {
            perror(0);
            kill(getpid(), SIGTERM);
        }
    }
    if (read_bytes == 0) {
        return ;
    }

    buffer[read_bytes] = '\0';

    size_t i = 0;
    int is_line_prefixed = 0;
    while (buffer[i]) {
        if (!is_line_prefixed) {
            write(STDOUT_FILENO, prefix, prefix_len);
            is_line_prefixed = 1;
        }
        if (buffer[i] == '\n') {
            is_line_prefixed = 0;
        }
        write(STDOUT_FILENO, buffer + i, 1);
        ++i;
    }
    assert(i > 0);
    if (buffer[i - 1] != '\n') {
        write(STDOUT_FILENO, "\n", 1);
    }
}

static void obj__wait_prefix_message(obj_t self) {
    static pid_t pid_prev;
    static int prefix_len_prev;
    static char prefix[256];
    if (self->pid > 0) {
        if (pid_prev == self->pid) {
            snprintf(prefix, sizeof(prefix), "%*s", prefix_len_prev, "");
            obj__wait_prefix_message_helper(self->pid_pipe_stdout[0], prefix, prefix_len_prev);
        } else {
            pid_prev = self->pid;
            obj__describe_time_stamp_prefix(self, prefix, sizeof(prefix));
            prefix_len_prev = strlen(prefix);
            obj__wait_prefix_message_helper(self->pid_pipe_stdout[0], prefix, prefix_len_prev);
        }
        if (pid_prev == self->pid) {
            snprintf(prefix, sizeof(prefix), "%*s", prefix_len_prev, "");
            obj__wait_prefix_message_helper(self->pid_pipe_stderr[0], prefix, prefix_len_prev);
        } else {
            pid_prev = self->pid;
            obj__describe_time_stamp_prefix(self, prefix, sizeof(prefix));
            prefix_len_prev = strlen(prefix);
            obj__wait_prefix_message_helper(self->pid_pipe_stderr[0], prefix, prefix_len_prev);
        }
    }
}

static void obj__wait(obj_t self) {
    int wstatus = -1;
    pid_t waited_pid = waitpid(self->pid, &wstatus, 0);
    self->is_running = NOT_RUNNING;
    obj__wait_prefix_message(self);

    if (waited_pid == -1) {
        self->last_run_result = RUN_RESULT_FAILED;
        perror(0);
    } else {
        assert(waited_pid == self->pid);
        obj__wait_status_helper(self, wstatus);
    }
}

static void obj__wait_no_hang(obj_t self) {
    int wstatus = -1;
    pid_t pid = waitpid(self->pid, &wstatus, WNOHANG);
    obj__wait_prefix_message(self);

    if (pid == -1) {
        self->pid = 0;
        self->is_running = NOT_RUNNING;
        self->last_run_result = RUN_RESULT_FAILED;
        perror(0);
    } else if (pid > 0) {
        assert(pid == self->pid);

        obj__wait_status_helper(self, wstatus);
    }
}

static void obj__run_sh_sync(obj_t self) {
    self->time_ran = builder__get_time_stamp();
    self->pid = fork();
    if (self->pid < 0) {
        perror(0);
        exit(1);
    }
    if (self->pid == 0) {
        close(self->pid_pipe_stdout[0]);
        if (dup2(self->pid_pipe_stdout[1], STDOUT_FILENO) == -1) {
            perror(0);
            exit(1);
        }
        close(self->pid_pipe_stdout[1]);
        close(self->pid_pipe_stderr[0]);
        if (dup2(self->pid_pipe_stderr[1], STDERR_FILENO) == -1) {
            perror(0);
            exit(1);
        }
        close(self->pid_pipe_stderr[1]);
        execl("/usr/bin/sh", "sh", "-c", ((obj_sh_t) self)->cmd_line, NULL);
        perror(0);
        exit(1);
    }

    char buffer[256];
    char prefix[32];
    obj__describe_time_stamp_prefix(self, prefix, sizeof(prefix));
    self->describe_short(self, buffer, sizeof(buffer));
    printf("%s%s\n", prefix, buffer);

    obj__wait(self);
}

static void obj__run_sh_async_wait(obj_t self) {
    obj__wait_no_hang(self);
    if (self->is_running == NOT_RUNNING) {
        async_obj__remove(self);
    }
}

static void obj__run_sh_async_start(obj_t self) {
    self->is_running = IS_RUNNING;

    if (self->pid > 0) {
        obj__wait_no_hang(self);
        if (self->is_running == IS_RUNNING) {
            obj__kill(self);
        }
        async_obj__remove(self);
    }

    self->time_ran = builder__get_time_stamp();
    self->pid = fork();

    if (self->pid < 0) {
        perror(0);
        exit(1);
    }
    if (self->pid == 0) {
        execl("/usr/bin/sh", "sh", "-c", ((obj_sh_t) self)->cmd_line, NULL);
        perror(0);
        exit(1);
    }

    char buffer[256];
    char prefix[32];
    obj__describe_time_stamp_prefix(self, prefix, sizeof(prefix));
    self->describe_short(self, buffer, sizeof(buffer));
    printf("%s%s\n", prefix, buffer);

    async_obj__add(self);
}

static void obj__run_sh(obj_t self, obj_t parent) {
    if (self->collector) {
        if (self->collector == parent) {
            if (self->is_running) {
                obj__run_sh_async_wait(self);
            }
            // todo: check if should be rerun, some logic for this already exists in obj__timed_run
        } else {
            obj__run_sh_async_start(self);
        }
    } else {
        obj__run_sh_sync(self);
    }
}

static void obj__run_file_modified(obj_t self, obj_t parent) {
    obj_file_modified_t file_modified = (obj_file_modified_t) self;
    (void) parent;

    struct stat file_info;
    if (stat(file_modified->path, &file_info) == -1) {
        self->last_run_result = RUN_RESULT_FAILED;
    } else if (self->time_ran == 0 || 0 < difftime(file_info.st_mtime, (time_t) self->time_ran)) {
        self->last_run_result = RUN_RESULT_SUCESSS;
        self->time_ran = (double) file_info.st_mtime;
    }
}

static void obj__run_oscillator(obj_t self, obj_t parent) {
    obj_oscillator_t oscillator = (obj_oscillator_t) self;
    if (self->time_ran == 0) {
        self->time_ran = parent->time_ran;
        self->last_run_result = RUN_RESULT_SUCESSS;
        return ;
    }

    const double periodicity_s = oscillator->periodicity_ms / 1000.0;
    const double minimum_periodicity_s = 0.1;
    if (periodicity_s < minimum_periodicity_s) {
        self->time_ran = parent->time_ran;
        self->last_run_result = RUN_RESULT_SUCESSS;
        return ;
    }

    const double epsilon_s = minimum_periodicity_s / 10.0; // to avoid modding down a whole period
    const double delta_time_last_successful_run_s = self->time_ran - g_time_init + epsilon_s;
    const double last_successful_run_mod_s = delta_time_last_successful_run_s - fmod(delta_time_last_successful_run_s, periodicity_s);
    const double delta_time_current_s = builder__get_time_stamp() - g_time_init;
    if (last_successful_run_mod_s + periodicity_s < delta_time_current_s) {
        self->time_ran = parent->time_ran;
        self->last_run_result = RUN_RESULT_SUCESSS;
    }
}

static void obj__run_time(obj_t self, obj_t parent) {
    (void) parent;

    self->last_run_result = RUN_RESULT_SUCESSS;
    self->time_ran = builder__get_time_stamp();
}

static void obj__run_list(obj_t self, obj_t parent) {
    (void) self;
    (void) parent;
    assert(0 && "lists should be used as temporary containers to create programs with");
}

static void obj__describe_short_sh(obj_t self, char* buffer, int buffer_size) {
    obj_sh_t process = (obj_sh_t) self;
    snprintf(
        buffer, buffer_size,
        "/usr/bin/sh -c \"%s\", expected status code: %d",
        process->cmd_line,
        self->success_status_code
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
    snprintf(buffer, buffer_size, "%.3lf", self->time_ran);
}

static void obj__describe_short_list(obj_t self, char* buffer, int buffer_size) {
    assert(0 && "lists should be used as temporary containers to create programs with");
    (void) self;
    (void) buffer;
    (void) buffer_size;
}

static void obj__destroy(obj_t self) {
    if (self->pid_pipe_stderr[0]) {
        close(self->pid_pipe_stderr[0]);
    }
    if (self->pid_pipe_stderr[1]) {
        close(self->pid_pipe_stderr[1]);
    }
    if (self->pid_pipe_stdout[0]) {
        close(self->pid_pipe_stdout[0]);
    }
    if (self->pid_pipe_stdout[1]) {
        close(self->pid_pipe_stdout[1]);
    }
    if (self->pid > 0) {
        obj__kill(self);
    }
    self->destroy(self);
}

static void obj__describe_long_sh(obj_t self, char* buffer, int buffer_size) {
    obj_sh_t process = (obj_sh_t) self;
    snprintf(buffer, buffer_size,
        "%s SH"
        "\n%s"
        "%s"
        "\nExpected status code: %d",
        self->collector ? "ASYNC" : "SYNC",
        process->cmd_line,
        self->collector ? (self->is_running == IS_RUNNING ? "\nRunning" : "\nNot running") : "",
        self->success_status_code
    );
}

static void obj__describe_long_file_modified(obj_t self, char* buffer, int buffer_size) {
    obj_file_modified_t file = (obj_file_modified_t) self;
    static char buffer2[128];
    if (self->time_ran == 0) {
        snprintf(buffer2, sizeof(buffer2), "Time modified: -");
    } else {
        time_t time_ran_successfully_time_t = (time_t) self->time_ran;
        struct tm* t = localtime(&time_ran_successfully_time_t);
        snprintf(buffer2, sizeof(buffer2), "Time modified: %02d/%02d/%d %02d:%02d:%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
    }
    snprintf(
        buffer, buffer_size,
        "FILE MODIFIED"
        "\n%s"
        "\n%s"
        ,
        file->path,
        buffer2
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

static void obj__describe_long_list(obj_t self, char* buffer, int buffer_size) {
    assert(0 && "lists should be used as temporary containers to create programs with");
    (void) self;
    (void) buffer;
    (void) buffer_size;
}

static void obj__destroy_sh(obj_t self) {
    (void) self;
    obj_sh_t obj_sh = (obj_sh_t) self;
    if (obj_sh->cmd_line) {
        free(obj_sh->cmd_line);
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
        assert(0 && "Handler is not registered for this signal.");
    }
}

static void async_obj__add(obj_t self) {
    assert(async_obj_top++ < ARRAY_SIZE(async_obj));
    for (size_t i = 0; i < ARRAY_SIZE(async_obj); ++i) {
        if (async_obj[i] == 0) {
            async_obj[i] = self;
            break ;
        }
    }
}

static void async_obj__remove(obj_t self) {
    assert(async_obj_top-- > 0);
    for (size_t i = 0; i < ARRAY_SIZE(async_obj); ++i) {
        if (async_obj[i] == self) {
            async_obj[i] = 0;
            break ;
        }
    }
}

void builder__init() {
    struct sigaction act = { 0 };
    act.sa_handler = &async_obj__signal_handler;
    sigaction(SIGTERM, &act, 0);

    struct timespec t;
    clock_getres(CLOCK_REALTIME, &t);
    g_tick_resolution = (double) t.tv_sec + t.tv_nsec / 1000000000.0;

    g_time_init = builder__get_time_stamp();
}

double builder__get_time_stamp() {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (t.tv_sec * 1000000000 + t.tv_nsec) * g_tick_resolution;
}

double builder__get_time_stamp_init() {
    return g_time_init;
}

static void obj__timed_run(obj_t self, obj_t parent) {
    if (self->is_locked == 1) {
        return ;
    }

    if (parent) {
        if (self->outputs_top > 0) {
            int found_older_output = 0;
            for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
                obj_t output = self->outputs[output_index];
                if (output->time_ran < parent->time_ran) {
                    found_older_output = 1;
                    break ;
                }
            }
            if (!found_older_output) {
                return ;
            }
        }
    }

    if (self->collector) {
        for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
            obj_t output = self->outputs[output_index];
            output->is_locked = 1;
        }
    }
    
    self->last_run_result = RUN_RESULT_NO_CHANGE;
    self->run(self, parent);

    if (self->collector && self->is_running == 0) {
        for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
            obj_t output = self->outputs[output_index];
            output->is_locked = 0;
        }
    }

    ++self->number_of_times_ran_total;
    switch (self->last_run_result) {
    case RUN_RESULT_NO_CHANGE: {
    } break ;
    case RUN_RESULT_SUCESSS: {
        ++self->number_of_times_ran_successfully;
    } break ;
    case RUN_RESULT_FAILED: {
        ++self->number_of_times_ran_failed;
    } break ;
    default: assert(0);
    }

    for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
        obj_t output = self->outputs[output_index];
        if (output->time_ran < self->time_ran) {
            obj__timed_run(output, self);
        }
    }
}

void obj__run(obj_t self) {
    obj__timed_run(self, 0);
}

void obj__describe_short(obj_t self, char* buffer, int buffer_size) {
    self->describe_short(self, buffer, buffer_size);
}

void obj__describe_long(obj_t self, char* buffer, int buffer_size) {
    self->describe_long(self, buffer, buffer_size);
}

static obj_t obj__alloc(size_t size) {
    obj_t result = calloc(1, size);

    return result;
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
    // if (result->base.last_run_result != RUN_RESULT_FAILED) {
    //     ++result->base.number_of_times_ran_successfully;
    // }

    if (opt_inputs) {
        obj__push_input((obj_t) result, opt_inputs);
    }

    return (obj_t) result;
}

const char* obj__file_modified_path(obj_t self) {
    return ((obj_file_modified_t) self)->path;
}

obj_t obj__sh(obj_t opt_inputs, obj_t opt_outputs, obj_t collector_if_async, int success_status_code, const char* cmd_line, ...) {
    obj_sh_t result = (obj_sh_t) obj__alloc(sizeof(*result));

    result->base.run            = &obj__run_sh;
    result->base.describe_short = &obj__describe_short_sh;
    result->base.describe_long  = &obj__describe_long_sh;
    result->base.destroy        = &obj__destroy_sh;

    result->base.success_status_code = success_status_code;

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

    if (opt_outputs) {
        obj__push_input(opt_outputs, (obj_t) result);
    }

    if (opt_inputs) {
        obj__push_input((obj_t) result, opt_inputs);
    }

    result->base.collector = collector_if_async;

    if (collector_if_async) {
        obj__push_input((obj_t) result, collector_if_async);
    }

    if (pipe(result->base.pid_pipe_stdout) == -1) {
        perror(0);
        obj__destroy((obj_t) result);
        return 0;
    }
    if (pipe(result->base.pid_pipe_stderr) == -1) {
        perror(0);
        obj__destroy((obj_t) result);
        return 0;
    }
    if (fcntl(result->base.pid_pipe_stdout[0], F_SETFL, fcntl(result->base.pid_pipe_stdout[0], F_GETFL) | O_NONBLOCK) == -1) {
        perror(0);
        return 0;
    }
    if (fcntl(result->base.pid_pipe_stderr[0], F_SETFL, fcntl(result->base.pid_pipe_stderr[0], F_GETFL) | O_NONBLOCK) == -1) {
        perror(0);
        return 0;
    }

    return opt_outputs ? opt_outputs : (obj_t) result;
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
