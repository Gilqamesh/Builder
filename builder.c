#include "builder.h"

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

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

extern char** environ;

struct compiler {
    const char*  path;
};

struct module_file {
    char* file_path;
    const char* opt_compiler_path;

    uint32_t cflags_top;
    uint32_t cflags_size;
    char**   cflags;

    uint32_t       dependencies_top;
    uint32_t       dependencies_size;
    module_file_t* dependencies;

    /**
     * 0  - hasn't been compiled yet
     * >0 - compiling
     * <0 - compiled
    */
    int32_t is_compiled;

    uint64_t time_last_modified;
    uint64_t time_last_compiled;
};

struct module {
    uint32_t       files_top;
    uint32_t       files_size;
    module_file_t* files;

    uint32_t        dependencies_top;
    uint32_t        dependencies_size;
    module_t* dependencies;

    uint32_t cflags_top;
    uint32_t cflags_size;
    char**   cflags;

    uint32_t lflags_top;
    uint32_t lflags_size;
    char**   lflags;

    /**
     * Can be used for anything like circular dependency check
     * 'transient_flag_is_used' must be cleared when done using it
    */
    int32_t transient_flag;
    int32_t transient_flag_is_used;

    /**
     * 0 if needs relink
     * 1 if does not need relink
    */
    int32_t relink_status;
};

static void module__vprepend_cflag(module_t self, const char* cflag_printf_format, va_list ap);
static void module__vappend_cflag(module_t self, const char* cflag_printf_format, va_list ap);
static void module__vprepend_lflag(module_t self, const char* lflag_printf_format, va_list ap);
static void module__vappend_lflag(module_t self, const char* lflag_printf_format, va_list ap);
static void module__collect_lflags(module_t self, module_t append_to, int32_t explore);
static void module__collect_cflags(module_t self, module_file_t append_to, int32_t explore);
static int32_t module__check_relink_status(module_t self, int32_t explore);

static module_file_t module_file__create(const char* opt_compiler_path, const char* file_path_printf_format, va_list ap);
static void module_file__destroy(module_file_t self);
static void module_file__vprepend_cflag(module_file_t self, const char* cflag_printf_format, va_list ap);
static void module_file__vappend_cflag(module_file_t self, const char* cflag_printf_format, va_list ap);
static void module_file__update_last_modified(module_file_t self);
static uint64_t module_file__latest_last_modified_dependency(module_file_t self);

static uint64_t time__current();

static int32_t my_vasprintf(char** strp, const char* format, va_list ap);
static int32_t my_asprintf(char** strp, const char* format, ...);

static void module__vprepend_cflag(module_t self, const char* cflag_printf_format, va_list ap) {
    module__vappend_cflag(self, cflag_printf_format, ap);

    assert(self->cflags_top >= 2);
    char* tmp = self->cflags[self->cflags_top - 2];
    for (uint32_t cflag_index = self->cflags_top - 2; cflag_index > 0; --cflag_index) {
        self->lflags[cflag_index] = self->cflags[cflag_index - 1];
    }
    self->cflags[0] = tmp;
}

static void module__vappend_cflag(module_t self, const char* cflag_printf_format, va_list ap) {
    char* str = 0;
    my_vasprintf(&str, cflag_printf_format, ap);
    assert(str);
    uint32_t start_index = 0;
    uint32_t end_index   = 0;
    for (uint32_t str_index = 0; str[str_index]; ++str_index, ++end_index) {
        if (str[str_index] == ' ') {
            if (end_index != start_index) {
                ARRAY_ENSURE_TOP(self->cflags, self->cflags_top, self->cflags_size);
                self->cflags[self->cflags_top] = 0;
                my_asprintf(&self->cflags[self->cflags_top - 1], "%.*s", end_index - start_index, str + start_index);
                ++self->cflags_top;

                start_index = end_index + 1;
            } else {
                ++start_index;
            }
        }
    }
    if (end_index != start_index) {
        ARRAY_ENSURE_TOP(self->cflags, self->cflags_top, self->cflags_size);
        self->cflags[self->cflags_top] = 0;
        my_asprintf(&self->cflags[self->cflags_top - 1], "%.*s", end_index - start_index, str + start_index);
        ++self->cflags_top;
    }
    free(str);
}

static void module__vprepend_lflag(module_t self, const char* lflag_printf_format, va_list ap) {
    module__vappend_lflag(self, lflag_printf_format, ap);

    assert(self->lflags_top >= 2);
    char* tmp = self->lflags[self->lflags_top - 2];
    for (uint32_t lflag_index = self->lflags_top - 2; lflag_index > 0; --lflag_index) {
        self->lflags[lflag_index] = self->lflags[lflag_index - 1];
    }
    self->lflags[0] = tmp;
}

static void module__vappend_lflag(module_t self, const char* lflag_printf_format, va_list ap) {
    char* str = 0;
    my_vasprintf(&str, lflag_printf_format, ap);
    assert(str);
    uint32_t start_index = 0;
    uint32_t end_index   = 0;
    for (uint32_t str_index = 0; str[str_index]; ++str_index, ++end_index) {
        if (str[str_index] == ' ') {
            if (end_index != start_index) {
                ARRAY_ENSURE_TOP(self->lflags, self->lflags_top, self->lflags_size);
                self->lflags[self->lflags_top] = 0;
                my_asprintf(&self->lflags[self->lflags_top - 1], "%.*s", end_index - start_index, str + start_index);
                ++self->lflags_top;

                start_index = end_index + 1;
            } else {
                ++start_index;
            }
        }
    }
    if (end_index != start_index) {
        ARRAY_ENSURE_TOP(self->lflags, self->lflags_top, self->lflags_size);
        self->lflags[self->lflags_top] = 0;
        my_asprintf(&self->lflags[self->lflags_top - 1], "%.*s", end_index - start_index, str + start_index);
        ++self->lflags_top;
    }
    free(str);
}

static void module__collect_lflags(module_t self, module_t append_to, int32_t explore) {
    if (explore) {
        if (self->transient_flag) {
            return ;
        }
        assert(!self->transient_flag_is_used);
        /**
         * 1 - visited
         * 0 - not visited
        */
        self->transient_flag = 1;
        self->transient_flag_is_used = 1;

        for (uint32_t dependency_index = 0; dependency_index < self->dependencies_top; ++dependency_index) {
            module_t dependency = self->dependencies[dependency_index];
            module__collect_lflags(dependency, append_to, explore);
        }

        for (size_t lflag_index = 0; lflag_index < self->lflags_top - 1; ++lflag_index) {
            int32_t found = 0;
            for (uint32_t append_to_lflag_index = 0; append_to_lflag_index < append_to->lflags_top - 1; ++append_to_lflag_index) {
                if (strcmp(append_to->lflags[append_to_lflag_index], self->lflags[lflag_index]) == 0) {
                    found = 1;
                    break ;
                }
            }
            if (!found) {
                module__append_lflag(append_to, self->lflags[lflag_index]);
            }
        }
    } else {
        if (self->transient_flag == 0) {
            return ;
        }
        assert(self->transient_flag_is_used);

        for (uint32_t dependency_index = 0; dependency_index < self->dependencies_top; ++dependency_index) {
            module_t dependency = self->dependencies[dependency_index];
            module__collect_lflags(dependency, 0, explore);
        }

        self->transient_flag         = 0;
        self->transient_flag_is_used = 0;
    }
}

static void module__collect_cflags(module_t self, module_file_t append_to, int32_t explore) {
    if (explore) {
        if (self->transient_flag) {
            return ;
        }
        assert(!self->transient_flag_is_used);
        /**
         * 1 - visited
         * 0 - not visited
        */
        self->transient_flag = 1;
        self->transient_flag_is_used = 1;

        for (uint32_t dependency_index = 0; dependency_index < self->dependencies_top; ++dependency_index) {
            module_t dependency = self->dependencies[dependency_index];
            module__collect_cflags(dependency, append_to, explore);
        }

        for (size_t cflag_index = 0; cflag_index < self->cflags_top - 1; ++cflag_index) {
            int32_t found = 0;
            for (uint32_t append_to_cflag_index = 0; append_to_cflag_index < append_to->cflags_top - 1; ++append_to_cflag_index) {
                if (strcmp(append_to->cflags[append_to_cflag_index], self->cflags[cflag_index]) == 0) {
                    found = 1;
                    break ;
                }
            }
            if (!found) {
                module_file__append_cflag(append_to, self->cflags[cflag_index]);
            }
        }
    } else {
        if (self->transient_flag == 0) {
            return ;
        }
        assert(self->transient_flag_is_used);

        for (uint32_t dependency_index = 0; dependency_index < self->dependencies_top; ++dependency_index) {
            module_t dependency = self->dependencies[dependency_index];
            module__collect_cflags(dependency, 0, explore);
        }

        self->transient_flag         = 0;
        self->transient_flag_is_used = 0;
    }
}

static int32_t module__check_relink_status(module_t self, int32_t explore) {
    if (explore) {
        if (self->transient_flag) {
            return self->relink_status;
        }
        assert(!self->transient_flag_is_used);
        /**
         * 1 - visited
         * 0 - not visited
        */
        self->transient_flag = 1;
        self->transient_flag_is_used = 1;

        for (uint32_t dependency_index = 0; dependency_index < self->dependencies_top; ++dependency_index) {
            module_t dependency = self->dependencies[dependency_index];
            if (!module__check_relink_status(dependency, explore)) {
                return 0;
            }
        }
    } else {
        if (self->transient_flag == 0) {
            return self->relink_status;
        }
        assert(self->transient_flag_is_used);

        for (uint32_t dependency_index = 0; dependency_index < self->dependencies_top; ++dependency_index) {
            module_t dependency = self->dependencies[dependency_index];
            module__check_relink_status(dependency, explore);
        }

        self->transient_flag         = 0;
        self->transient_flag_is_used = 0;
        self->relink_status          = 1;
    }

    return self->relink_status;
}

static module_file_t module_file__create(const char* opt_compiler_path, const char* file_path_printf_format, va_list ap) {
    module_file_t result = calloc(1, sizeof(*result));
    if (!result) {
        return 0;
    }

    my_vasprintf(&result->file_path, file_path_printf_format, ap);
    result->opt_compiler_path = opt_compiler_path;
    module_file__update_last_modified(result);

    ARRAY_ENSURE_TOP(result->cflags, result->cflags_top, result->cflags_size);
    result->cflags[result->cflags_top++] = 0;

    return result;
}

static void module_file__destroy(module_file_t self) {
    if (self->file_path) {
        free(self->file_path);
    }

    if (self->cflags) {
        for (uint32_t cflag_index = 0; cflag_index < self->cflags_top; ++cflag_index) {
            if (self->cflags[cflag_index]) {
                free(self->cflags[cflag_index]);
            }
        }
        free(self->cflags);
    }

    free(self);
}

static void module_file__vprepend_cflag(module_file_t self, const char* cflag_printf_format, va_list ap) {
    module_file__vappend_cflag(self, cflag_printf_format, ap);

    assert(self->cflags_top >= 2);
    char* tmp = self->cflags[self->cflags_top - 2];
    for (uint32_t cflag_index = self->cflags_top - 2; cflag_index > 0; --cflag_index) {
        self->cflags[cflag_index] = self->cflags[cflag_index - 1];
    }
    self->cflags[0] = tmp;
}

static void module_file__vappend_cflag(module_file_t self, const char* cflag_printf_format, va_list ap) {
    char* str = 0;
    my_vasprintf(&str, cflag_printf_format, ap);
    assert(str);
    uint32_t start_index = 0;
    uint32_t end_index   = 0;
    for (uint32_t str_index = 0; str[str_index]; ++str_index, ++end_index) {
        if (str[str_index] == ' ') {
            if (end_index != start_index) {
                ARRAY_ENSURE_TOP(self->cflags, self->cflags_top, self->cflags_size);
                self->cflags[self->cflags_top] = 0;
                my_asprintf(&self->cflags[self->cflags_top - 1], "%.*s", end_index - start_index, str + start_index);
                ++self->cflags_top;

                start_index = end_index + 1;
            } else {
                ++start_index;
            }
        }
    }
    if (end_index != start_index) {
        ARRAY_ENSURE_TOP(self->cflags, self->cflags_top, self->cflags_size);
        self->cflags[self->cflags_top] = 0;
        my_asprintf(&self->cflags[self->cflags_top - 1], "%.*s", end_index - start_index, str + start_index);
        ++self->cflags_top;
    }
    free(str);
}

static void module_file__update_last_modified(module_file_t self) {
    struct stat file_info;
    if (stat(self->file_path, &file_info) == -1) {
        assert(0);
    }

    self->time_last_modified = (uint64_t) file_info.st_mtime;
    if (!self->opt_compiler_path) {
        // does not have IR, thus compilation time is synched to its last modified time
        self->time_last_compiled = self->time_last_modified;
    }            
}

static uint64_t module_file__latest_last_modified_dependency(module_file_t self) {
    module_file__update_last_modified(self);

    uint64_t latest_last_modified = self->time_last_modified;

    for (uint32_t dependency_index = 0; dependency_index < self->dependencies_top; ++dependency_index) {
        uint64_t latest_last_modified_dependency = module_file__latest_last_modified_dependency(self->dependencies[dependency_index]);
        if (latest_last_modified_dependency > latest_last_modified) {
            latest_last_modified = latest_last_modified_dependency;
        }
    }

    return latest_last_modified;
}

static uint64_t time__current() {
    return time(0);
}

static int32_t my_vasprintf(char** strp, const char* format, va_list ap) {
    const uint32_t max_cflag_len = 256;
    char* str = malloc(max_cflag_len);
    const int32_t bytes_needed = vsnprintf(str, max_cflag_len, format, ap);
    assert(bytes_needed >= 0);
    assert(bytes_needed < (int32_t) max_cflag_len);
    *strp = malloc(bytes_needed + 1);
    strncpy(*strp, str, bytes_needed + 1);
    free(str);

    return bytes_needed;
}

static int32_t my_asprintf(char** strp, const char* format, ...) {
    va_list ap;
    va_start(ap, format);

    int32_t result = my_vasprintf(strp, format, ap);

    va_end(ap);

    return result;
}

module_t module__create() {
    module_t result = calloc(1, sizeof(*result));

    ARRAY_ENSURE_TOP(result->cflags, result->cflags_top, result->cflags_size);
    result->cflags[result->cflags_top++] = 0;
    ARRAY_ENSURE_TOP(result->lflags, result->lflags_top, result->lflags_size);
    result->lflags[result->lflags_top++] = 0;

    return result;
}

void module__destroy(module_t self) {
    if (self->files) {
        for (uint32_t file_index = 0; file_index < self->files_top; ++file_index) {
            module_file__destroy(self->files[file_index]);
        }
        free(self->files);
    }

    if (self->dependencies) {
        free(self->dependencies);
    }

    if (self->cflags) {
        for (uint32_t cflag_index = 0; cflag_index < self->cflags_top; ++cflag_index) {
            if (self->cflags[cflag_index]) {
                free(self->cflags[cflag_index]);
            }
        }
        free(self->cflags);
    }

    if (self->lflags) {
        for (uint32_t lflag_index = 0; lflag_index < self->lflags_top; ++lflag_index) {
            if (self->lflags[lflag_index]) {
                free(self->lflags[lflag_index]);
            }
        }
        free(self->lflags);
    }

    free(self);
}

void module__prepend_cflag(module_t self, const char* cflag_printf_format, ...) {
    va_list ap;
    va_start(ap, cflag_printf_format);

    module__vprepend_cflag(self, cflag_printf_format, ap);

    va_end(ap);
}

void module__append_cflag(module_t self, const char* cflag_printf_format, ...) {
    va_list ap;
    va_start(ap, cflag_printf_format);

    module__vappend_cflag(self, cflag_printf_format, ap);

    va_end(ap);
}

void module__prepend_lflag(module_t self, const char* lflag_printf_format, ...) {
    va_list ap;
    va_start(ap, lflag_printf_format);

    module__vprepend_lflag(self, lflag_printf_format, ap);

    va_end(ap);
}

void module__append_lflag(module_t self, const char* lflag_printf_format, ...) {
    va_list ap;
    va_start(ap, lflag_printf_format);

    module__vappend_lflag(self, lflag_printf_format, ap);

    va_end(ap);
}

void module__add_dependency(module_t self, module_t dependency) {
    for (uint32_t dependency_index = 0; dependency_index < self->dependencies_top; ++dependency_index) {
        if (self->dependencies[dependency_index] == dependency) {
            return ;
        }
    }

    ARRAY_ENSURE_TOP(self->dependencies, self->dependencies_top, self->dependencies_size);
    self->dependencies[self->dependencies_top++] = dependency;
}

void module__compile_with_dependencies(module_t self) {
    for (uint32_t dependency_index = 0; dependency_index < self->dependencies_top; ++dependency_index) {
        module_t dependency = self->dependencies[dependency_index];
        module__compile_with_dependencies(dependency);
    }

    for (uint32_t file_index = 0; file_index < self->files_top; ++file_index) {
        module_file_t module_file = self->files[file_index];
        
        module_file__update_last_modified(module_file);

        if (!module_file->opt_compiler_path) {
            continue;
        }

        if (module_file->is_compiled > 0) {
            // compiling -> nothing to do
            continue ;
        }

        if (module_file->is_compiled < 0) {
            // compiled -> check whether or not dependencies are changed since last compilation

            uint64_t latest_last_modified_dependency = module_file__latest_last_modified_dependency(module_file);
            if (latest_last_modified_dependency <= module_file->time_last_compiled) {
                continue ;
            }
        } else {
            module_file__prepend_cflag(module_file, module_file->opt_compiler_path);
            module__collect_cflags(self, module_file, 1);
            module__collect_cflags(self, module_file, 0);
        }

        for (uint32_t cflag_index = 0; cflag_index < module_file->cflags_top - 1; ++cflag_index) {
            printf("%s ", module_file->cflags[cflag_index]);
        }
        printf("\n");
        pid_t pid = fork();
        if (pid == -1) {
            perror(0);
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            execve(module_file->opt_compiler_path, (char* const*) module_file->cflags, environ);
            perror(0);
            exit(EXIT_FAILURE);
        }

        module_file->is_compiled = pid;
        module_file->time_last_compiled = time__current();

        // module needs to be relinked
        self->relink_status = 0;
    }
}

void module__wait_for_compilation(module_t self) {
    for (uint32_t dependency_index = 0; dependency_index < self->dependencies_top; ++dependency_index) {
        module_t dependency = self->dependencies[dependency_index];
        module__wait_for_compilation(dependency);
    }

    for (uint32_t file_index = 0; file_index < self->files_top; ++file_index) {
        module_file_t file = self->files[file_index];
        if (file->is_compiled > 0) {
            pid_t file_pid = waitpid(file->is_compiled, 0, 0);
            assert(file_pid == file->is_compiled);
            file->is_compiled = -1;
        }
    }
}

void module__link(module_t self, const char* linker_path, int* optional_status_code) {
    int32_t relink_status = module__check_relink_status(self, 1);
    module__check_relink_status(self, 0);
    if (relink_status) {
        return ;
    }

    module_t fake_module = module__create();
    module__append_lflag(fake_module, "%s",  linker_path);
    module__collect_lflags(self, fake_module, 1);
    module__collect_lflags(self, fake_module, 0);
    for (uint32_t arg_index = 0; arg_index < fake_module->lflags_top - 1; ++arg_index) {
        printf("%s ", fake_module->lflags[arg_index]);
    }
    printf("\n");

    pid_t pid = fork();
    if (pid == -1) {
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        execve(linker_path, (char* const*) fake_module->lflags, environ);
        perror(0);
        exit(EXIT_FAILURE);
    }
    waitpid(pid, optional_status_code, 0);
    if (optional_status_code && WIFEXITED(*optional_status_code)) {
        *optional_status_code = WEXITSTATUS(*optional_status_code);
    }

    module__destroy(fake_module);
}

module_file_t module__add_file(module_t self, const char* opt_file_compiler_path, const char* file_path_printf_format, ...) {
    va_list ap;
    va_start(ap, file_path_printf_format);
    module_file_t result = module_file__create(opt_file_compiler_path, file_path_printf_format, ap);
    va_end(ap);

    if (!result) {
        return 0;
    }

    ARRAY_ENSURE_TOP(self->files, self->files_top, self->files_size);
    self->files[self->files_top++] = result;

    return result;
}

void module_file__prepend_cflag(module_file_t self, const char* cflag_printf_format, ...) {
    va_list ap;
    va_start(ap, cflag_printf_format);

    module_file__vprepend_cflag(self, cflag_printf_format, ap);

    va_end(ap);
}

void module_file__append_cflag(module_file_t self, const char* cflag_printf_format, ...) {
    va_list ap;
    va_start(ap, cflag_printf_format);

    module_file__vappend_cflag(self, cflag_printf_format, ap);

    va_end(ap);
}

void module_file__add_dependency(module_file_t self, module_file_t file) {
    for (uint32_t dependency_index = 0; dependency_index < self->dependencies_top; ++dependency_index) {
        if (self->dependencies[dependency_index] == file) {
            return ;
        }
    }

    ARRAY_ENSURE_TOP(self->dependencies, self->dependencies_top, self->dependencies_size);
    self->dependencies[self->dependencies_top++] = file;
}
