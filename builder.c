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

typedef struct obj_wait {
    struct obj base;
} *obj_wait_t;

typedef struct obj_list {
    struct obj base;
} *obj_list_t;

static obj_t obj__alloc(size_t size);
static void obj__push_input(obj_t self, obj_t input);
static void obj__check_for_cyclic_input(obj_t self);
static void obj__kill(obj_t self);
static int  obj__read(obj_t self, char* buffer, int buffer_size);
static void obj__describe_time_stamp_prefix(obj_t self, char* buffer, int buffer_size);
static void obj__set_start(obj_t self, double time);
static void obj__set_success(obj_t self, double time);
static void obj__set_fail(obj_t self, double time);
static void obj__set_finish(obj_t self, double time);

static void obj__run_sh(obj_t self);
static void obj__run_file_modified(obj_t self);
static void obj__run_oscillator(obj_t self);
static void obj__run_time(obj_t self);
static void obj__run_wait(obj_t self, int hang);
static void obj__run_wait_no_hang(obj_t self);
static void obj__run_wait_hang(obj_t self);
static void obj__run_wait_impl(obj_t self, int hang);
static void obj__run_list(obj_t self);

static void obj__describe_short_sh(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_file_modified(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_oscillator(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_time(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_wait(obj_t self, char* buffer, int buffer_size);
static void obj__describe_short_list(obj_t self, char* buffer, int buffer_size);

static void obj__describe_long_sh(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_file_modified(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_oscillator(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_time(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_wait(obj_t self, char* buffer, int buffer_size);
static void obj__describe_long_list(obj_t self, char* buffer, int buffer_size);

static void obj__destroy_sh(obj_t self);
static void obj__destroy_file_modified(obj_t self);
static void obj__destroy_oscillator(obj_t self);
static void obj__destroy_time(obj_t self);
static void obj__destroy_wait(obj_t self);
static void obj__destroy_list(obj_t self);

static obj_t obj__alloc(size_t size) {
    obj_t result = calloc(1, size);

    return result;
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

static void obj__check_for_cyclic_input(obj_t self) {
    assert(self->transient_flag == 0);
    if (obj__check_for_cyclic_input_helper(self)) {
        obj__print_input_graph_helper(self, 0);
        assert(0);
    }
}

static void obj__describe_time_stamp_prefix(obj_t self, char* buffer, int buffer_size) {
    const double time_since_init = builder__get_time_stamp() - g_time_init;
    snprintf(buffer, buffer_size, "%.2fs [%d] ", time_since_init, self->pid);
}

static void obj__kill(obj_t self) {
    if (self->pid > 0) {
        assert(self->time_ran_finish <= self->time_ran_start);

        if (kill(self->pid, SIGTERM) == -1) {
            printf("kill failed, %s\n", strerror(errno));
        }
        // if (self->time_ran_finish < self->time_ran_start) {
        //     if (kill(self->pid, SIGKILL) == -1) {
        //         printf("kill failed, %s\n", strerror(errno));
        //     }
        // }
        
        const double time_cur = builder__get_time_stamp();
        obj__set_fail(self, time_cur);
        obj__set_finish(self, time_cur);
    }
}

static int obj__read_helper(int fd, char* buffer, int buffer_size, char* prefix) {
    if (buffer_size < 2) {
        return 0;
    }
    buffer[0] = '\0';

    static char local_buffer[1024];
    ssize_t read_bytes = read(fd, local_buffer, sizeof(local_buffer) - 1);
    if (read_bytes == -1) {
        if (errno == EAGAIN) {
            return 0;
        } else {
            int bytes_read = snprintf(
                buffer,
                buffer_size,
                "%s%s\n",
                prefix,
                strerror(errno)
            );
            if (bytes_read < 0) {
                perror(0);
                exit(1);
            }
            return bytes_read < buffer_size - 1 ? bytes_read : buffer_size - 1;
        }
    } else if (read_bytes == 0) {
        return 0;
    }

    local_buffer[read_bytes] = '\0';

    size_t local_buffer_index = 0;
    int is_line_prefixed = 0;
    int buffer_index = 0;
    while (local_buffer[local_buffer_index]) {
        if (!is_line_prefixed) {
            int bytes_written = snprintf(
                buffer,
                buffer_size - buffer_index,
                "%s",
                prefix
            );
            if (bytes_written < 0) {
                break ;
            }
            buffer_index += bytes_written;
            is_line_prefixed = 1;
        }
        if (local_buffer[local_buffer_index] == '\n') {
            is_line_prefixed = 0;
        }
        
        if (buffer_size <= buffer_index) {
            break ;
        }
        buffer[buffer_index++] = local_buffer[local_buffer_index++];
    }

    if (buffer_size <= buffer_index) {
        buffer_index = buffer_size;        
    }

    buffer[buffer_index - 1] = '\0';
    buffer[buffer_index - 2] = '\n';

    return buffer_index - 1;
}

static int obj__read(obj_t self, char* buffer, int buffer_size) {
    static pid_t pid_prev;
    static int prefix_len_prev;
    static char prefix[256];
    int bytes_read = 0;
    if (self->pid > 0) {
        if (pid_prev == self->pid) {
            snprintf(prefix, sizeof(prefix), "%*s", prefix_len_prev, "");
            bytes_read = obj__read_helper(self->pid_pipe_stdout[0], buffer, buffer_size, prefix);
        } else {
            obj__describe_time_stamp_prefix(self, prefix, sizeof(prefix));
            prefix_len_prev = strlen(prefix);
            bytes_read = obj__read_helper(self->pid_pipe_stdout[0], buffer, buffer_size, prefix);
        }
        if (pid_prev == self->pid) {
            snprintf(prefix, sizeof(prefix), "%*s", prefix_len_prev, "");
            bytes_read = obj__read_helper(self->pid_pipe_stderr[0], buffer, buffer_size, prefix);
        } else {
            obj__describe_time_stamp_prefix(self, prefix, sizeof(prefix));
            prefix_len_prev = strlen(prefix);
            bytes_read = obj__read_helper(self->pid_pipe_stderr[0], buffer, buffer_size, prefix);
        }
    }

    pid_prev = self->pid;

    return bytes_read;
}

static void obj__run_sh(obj_t self) {
    obj__set_start(self, builder__get_time_stamp());

    // assert(self->pid == 0);
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

    async_obj__add(self);
}

static void obj__run_file_modified(obj_t self) {
    obj_file_modified_t file_modified = (obj_file_modified_t) self;

    const double time_cur = builder__get_time_stamp();
    struct stat file_info;
    if (stat(file_modified->path, &file_info) == -1) {
        obj__set_start(self, time_cur);
        obj__set_fail(self, time_cur);
        obj__set_finish(self, time_cur);
    } else if (self->time_ran_success == 0 || 0 < difftime(file_info.st_mtime, (time_t) self->time_ran_success)) {
        const double time_modified = (double) file_info.st_mtime;
        obj__set_start(self, time_modified);
        obj__set_success(self, time_modified);
        obj__set_finish(self, time_modified);
    }
}

static void obj__run_oscillator(obj_t self) {
    const double time_cur = builder__get_time_stamp();
    obj__set_start(self, time_cur);

    obj_oscillator_t oscillator = (obj_oscillator_t) self;
    if (self->time_ran_success == 0) {
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
    const double delta_time_last_successful_run_s = self->time_ran_success - g_time_init + epsilon_s;
    const double last_successful_run_mod_s = delta_time_last_successful_run_s - fmod(delta_time_last_successful_run_s, periodicity_s);
    const double delta_time_current_s = builder__get_time_stamp() - g_time_init;
    if (last_successful_run_mod_s + periodicity_s < delta_time_current_s) {
        obj__set_success(self, time_cur);
    }
    obj__set_finish(self, time_cur);
}

static void obj__run_time(obj_t self) {
    const double time_cur = builder__get_time_stamp();
    obj__set_start(self, time_cur);
    obj__set_success(self, time_cur);
    obj__set_finish(self, time_cur);
}

static void obj__run_wait_impl(obj_t self, int hang) {
    assert(self->pid > 0);
    int wstatus = -1;
    pid_t waited_pid = waitpid(self->pid, &wstatus, hang ? 0 : WNOHANG);
    static char buffer[1024];
    int bytes_read = obj__read(self, buffer, sizeof(buffer));
    if (0 < bytes_read) {
        printf("%s", buffer);
    }
    if (waited_pid == -1) {
        const double time_cur = builder__get_time_stamp();
        obj__set_fail(self, time_cur);
        obj__set_finish(self, time_cur);
    } else if (waited_pid > 0) {
        assert(waited_pid == self->pid);
        char prefix[32];
        obj__describe_time_stamp_prefix(self, prefix, sizeof(prefix));
        if (WIFEXITED(wstatus)) {
            wstatus = WEXITSTATUS(wstatus);
            if (self->success_status_code == wstatus) {
                obj__set_success(self, builder__get_time_stamp());
                printf("%sfinished successfully with status code: %d\n", prefix, wstatus);
            } else {
                obj__set_fail(self, builder__get_time_stamp());
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
        obj__set_finish(self, builder__get_time_stamp());
    }
}

static void obj__run_wait(obj_t self, int hang) {
    for (size_t input_index = 0; input_index < self->inputs_top; ++input_index) {
        obj_t input = self->inputs[input_index];
        if (input->time_ran_finish < input->time_ran_start) {
            obj__run_wait_impl(input, hang);
        }
    }

    for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
        obj_t output = self->outputs[output_index];
        self->time_ran_start = fmax(self->time_ran_start, output->time_ran_start);
        self->time_ran_success = fmax(self->time_ran_success, output->time_ran_success);
        self->time_ran_fail = fmax(self->time_ran_fail, output->time_ran_fail);
        self->time_ran_finish = fmax(self->time_ran_finish, output->time_ran_finish);
    }
}

static void obj__run_wait_no_hang(obj_t self) {
    obj__run_wait(self, 0);
}

static void obj__run_wait_hang(obj_t self) {
    obj__run_wait(self, 1);
}

static void obj__run_list(obj_t self) {
    (void) self;
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
    snprintf(buffer, buffer_size, "%.3lf", self->time_ran_success);
}

static void obj__describe_short_wait(obj_t self, char* buffer, int buffer_size) {
    (void) self;
    snprintf(buffer, buffer_size, "TIME");
}

static void obj__describe_short_list(obj_t self, char* buffer, int buffer_size) {
    assert(0 && "lists should be used as temporary containers to create programs with");
    (void) self;
    (void) buffer;
    (void) buffer_size;
}

static void obj__destroy(obj_t self) {
    if (self->pid > 0) {
        obj__kill(self);
        self->pid = 0;
    }
    if (self->pid_pipe_stderr[0]) {
        close(self->pid_pipe_stderr[0]);
        self->pid_pipe_stderr[0] = 0;
    }
    if (self->pid_pipe_stderr[1]) {
        close(self->pid_pipe_stderr[1]);
        self->pid_pipe_stderr[1] = 0;
    }
    if (self->pid_pipe_stdout[0]) {
        close(self->pid_pipe_stdout[0]);
        self->pid_pipe_stdout[0] = 0;
    }
    if (self->pid_pipe_stdout[1]) {
        close(self->pid_pipe_stdout[1]);
        self->pid_pipe_stdout[1] = 0;
    }
    self->destroy(self);
}

static void obj__describe_long_sh(obj_t self, char* buffer, int buffer_size) {
    obj_sh_t process = (obj_sh_t) self;
    snprintf(buffer, buffer_size,
        "SH"
        "\n%s"
        "\nExpected status code: %d",
        process->cmd_line,
        self->success_status_code
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

static void obj__describe_long_wait(obj_t self, char* buffer, int buffer_size) {
    (void) self;
    snprintf(buffer, buffer_size, "WAIT");
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
        obj_sh->cmd_line = 0;
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

static void obj__destroy_wait(obj_t self) {
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

static void obj__set_start(obj_t self, double time) {
    self->time_ran_start = time;
    ++self->number_of_times_ran_total;
    async_obj__add(self);
}

static void obj__set_finish(obj_t self, double time) {
    self->time_ran_finish = time;
    self->pid = 0;
    async_obj__remove(self);
}

static void obj__set_success(obj_t self, double time) {
    self->time_ran_success = time;
    ++self->number_of_times_ran_successfully;
}

static void obj__set_fail(obj_t self, double time) {
    self->time_ran_fail = time;
    ++self->number_of_times_ran_failed;
}

static void obj__run_helper(obj_t self) {
    double time_input = 0.0;
    for (size_t input_index = 0; input_index < self->inputs_top; ++input_index) {
        obj_t input = self->inputs[input_index];
        if (input->time_ran_success < input->time_ran_fail) {
            return ;
        }
        time_input = fmax(time_input, fmax(input->time_ran_start, input->time_ran_success));
    }
    
    if (0.0 < time_input && 0 < self->outputs_top) {
        int found_older_output = 0;
        for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
            obj_t output = self->outputs[output_index];
            if (fmax(output->time_ran_start, output->time_ran_success) < time_input) {
                found_older_output = 1;
                break ;
            }
        }
        if (!found_older_output) {
            return ;
        }
    }

    obj__kill(self);
    // self->time_ran_start = ;
    self->run(self);

    // backpropagate our time to inputs

    if (
        self->time_ran_success < self->time_ran_fail ||
        self->time_ran_start == 0.0
    ) {
        return ;
    }

    const double time_self = fmax(self->time_ran_success, self->time_ran_start);
    for (size_t output_index = 0; output_index < self->outputs_top; ++output_index) {
        obj_t output = self->outputs[output_index];
        if (fmax(output->time_ran_start, output->time_ran_success) < time_self) {
            obj__run_helper(output);
        }
    }
}

int obj__run(obj_t self) {
    obj__run_helper(self);
    if (self->time_ran_start == 0.0) {
        return 1;
    } else {
        if (self->time_ran_success < self->time_ran_fail) {
            return 1;
        } else if (self->time_ran_fail < self->time_ran_success) {
            return 0;
        } else if (self->time_ran_start <= self->time_ran_finish) {
            return 1;
        } else {
            return 1;
        }
    }
}

void obj__describe_short(obj_t self, char* buffer, int buffer_size) {
    self->describe_short(self, buffer, buffer_size);
}

void obj__describe_long(obj_t self, char* buffer, int buffer_size) {
    char self_describe_long[512];
    self->describe_long(self, self_describe_long, sizeof(self_describe_long));

    char run_status[32];
    if (self->time_ran_start == 0.0) {
        snprintf(run_status, sizeof(run_status), "Did not run");
    } else {
        if (self->time_ran_success < self->time_ran_fail) {
            snprintf(run_status, sizeof(run_status), "Failed");
        } else if (self->time_ran_fail < self->time_ran_success) {
            snprintf(run_status, sizeof(run_status), "Succeeded");
        } else if (self->time_ran_start <= self->time_ran_finish) {
            snprintf(run_status, sizeof(run_status), "Finished");
        } else {
            snprintf(run_status, sizeof(run_status), "Running");
        }
    }
    char abs_time_run_success[128];
    char rel_time_run_success[128];
    if (self->time_ran_success == 0) {
        snprintf(abs_time_run_success, sizeof(abs_time_run_success), "-");
        snprintf(rel_time_run_success, sizeof(rel_time_run_success), "-");
    } else {
        time_t time_ran_successfully_time_t = (time_t) self->time_ran_success;
        struct tm* t = localtime(&time_ran_successfully_time_t);
        snprintf(abs_time_run_success, sizeof(abs_time_run_success), "%02d/%02d/%d %02d:%02d:%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
        snprintf(rel_time_run_success, sizeof(rel_time_run_success), "%.2lf", self->time_ran_success - builder__get_time_stamp_init());
    }
    char abs_time_run_fail[128];
    char rel_time_run_fail[128];
    if (self->time_ran_fail == 0) {
        snprintf(abs_time_run_fail, sizeof(abs_time_run_fail), "-");
        snprintf(rel_time_run_fail, sizeof(rel_time_run_fail), "-");
    } else {
        time_t time_ran_successfully_time_t = (time_t) self->time_ran_fail;
        struct tm* t = localtime(&time_ran_successfully_time_t);
        snprintf(abs_time_run_fail, sizeof(abs_time_run_fail), "%02d/%02d/%d %02d:%02d:%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
        snprintf(rel_time_run_fail, sizeof(rel_time_run_fail), "%.2lf", self->time_ran_fail - builder__get_time_stamp_init());
    }
    char abs_time_run_start[128];
    char rel_time_run_start[128];
    if (self->time_ran_start == 0) {
        snprintf(abs_time_run_start, sizeof(abs_time_run_start), "-");
        snprintf(rel_time_run_start, sizeof(rel_time_run_start), "-");
    } else {
        time_t time_ran_successfully_time_t = (time_t) self->time_ran_start;
        struct tm* t = localtime(&time_ran_successfully_time_t);
        snprintf(abs_time_run_start, sizeof(abs_time_run_start), "%02d/%02d/%d %02d:%02d:%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
        snprintf(rel_time_run_start, sizeof(rel_time_run_start), "%.2lf", self->time_ran_start - builder__get_time_stamp_init());
    }
    char abs_time_run_finish[128];
    char rel_time_run_finish[128];
    if (self->time_ran_finish == 0) {
        snprintf(abs_time_run_finish, sizeof(abs_time_run_finish), "-");
        snprintf(rel_time_run_finish, sizeof(rel_time_run_finish), "-");
    } else {
        time_t time_ran_successfully_time_t = (time_t) self->time_ran_finish;
        struct tm* t = localtime(&time_ran_successfully_time_t);
        snprintf(abs_time_run_finish, sizeof(abs_time_run_finish), "%02d/%02d/%d %02d:%02d:%02d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
        snprintf(rel_time_run_finish, sizeof(rel_time_run_finish), "%.2lf", self->time_ran_finish - builder__get_time_stamp_init());
    }

    snprintf(
        buffer, buffer_size,
        "%s"
        "\nPid: %u"
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
        self->pid,
        run_status,
        abs_time_run_success,
        abs_time_run_fail,
        abs_time_run_start,
        abs_time_run_finish,
        rel_time_run_success,
        rel_time_run_fail,
        rel_time_run_start,
        rel_time_run_finish,
        self->number_of_times_ran_total,
        self->number_of_times_ran_successfully,
        self->number_of_times_ran_failed
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

    result->base.success_status_code = success_status_code;

    if (!cmd_line) {
        int dbg = 0;
        ++dbg;
    }
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
        obj__destroy((obj_t) result);
        return 0;
    }
    if (fcntl(result->base.pid_pipe_stderr[0], F_SETFL, fcntl(result->base.pid_pipe_stderr[0], F_GETFL) | O_NONBLOCK) == -1) {
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

obj_t obj__wait(obj_t input, int hang) {
    obj_wait_t result = (obj_wait_t) obj__alloc(sizeof(*result));

    result->base.run = hang ? &obj__run_wait_hang : &obj__run_wait_no_hang;
    result->base.describe_short = &obj__describe_short_wait;
    result->base.describe_long = &obj__describe_long_wait;
    result->base.destroy = &obj__destroy_wait;

    if (input) {
        obj__push_input((obj_t) result, input);
    }

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
