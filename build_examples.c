#include "builder.h"

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>

#define ARRAY_SIZE(array) (sizeof(array)/sizeof((array)[0]))

typedef struct supported_module {
    module_t module;
    const char* module_dir_path;
} supported_module_t;

static void supported_module__initialize(supported_module_t* self);

static void supported_module__execute(void* user_data);

static void module__initialize_builder_module();

static void print__supported_modules_names();

static const char* const c_compiler_path   = "/usr/bin/gcc";
static const char* const cpp_compiler_path = "/usr/bin/g++";

static const char* pwd;

static module_t builder_module;

static supported_module_t supported_modules[] = {
    {
        .module_dir_path = "examples/simple"
    },
    {
        .module_dir_path = "examples/shared_lib"
    },
    {
        .module_dir_path = "examples/multiple_compilers"
    }
};

static void supported_module__initialize(supported_module_t* self) {
    self->module = module__create();

    module_file_t build_example_ir_file  = module__add_file(self->module, 0, "%s/build_example.o", self->module_dir_path);
    module_file_t build_example_src_file = module__add_file(self->module, c_compiler_path, "%s/build_example.c", self->module_dir_path);
    module_file_t build_example_bin_file = module__add_file(self->module, 0, "%s/build_example", self->module_dir_path);

    module_file__append_cflag(build_example_src_file, "-g -c %s -o %s", module_file__path(build_example_src_file), module_file__path(build_example_ir_file));

    module_file__add_dependency(build_example_ir_file, build_example_src_file);
    module_file__add_dependency(build_example_bin_file, build_example_ir_file);

    module__append_lflag(self->module, "-o %s", module_file__path(build_example_bin_file));
    module__append_lflag(self->module, "%s", module_file__path(build_example_ir_file));

    assert(builder_module);
    module__add_dependency(self->module, builder_module);
}

static void supported_module__execute(void* user_data) {
    supported_module_t* supported_module = (supported_module_t*) user_data;
    printf("cd %s\n", supported_module->module_dir_path);
    if (chdir(supported_module->module_dir_path) == -1) {
        perror(0);
        return ;
    }

    printf("./build_example\n");
    if (execl("build_example", "build_example", 0) == -1) {
        perror(0);
    }
}

static void print__supported_modules_names() {
    fprintf(stderr, "Supported modules:\n");
    for (uint32_t sm_index = 0; sm_index < ARRAY_SIZE(supported_modules); ++sm_index) {
        fprintf(stderr, "    %s\n", supported_modules[sm_index].module_dir_path);
    }
}

static void module__initialize_builder_module() {
    builder_module = module__create();

    module_file_t builder_header = module__add_file(builder_module, 0, "builder.h");
    module_file_t builder_src    = module__add_file(builder_module, c_compiler_path, "builder.c");
    module_file_t builder_ir     = module__add_file(builder_module, 0, "builder.o");

    module_file__add_dependency(builder_src, builder_header);
    module_file__add_dependency(builder_ir, builder_src);

    module_file__append_cflag(builder_src, "-c %s -o %s", module_file__path(builder_src), module_file__path(builder_ir));

    module__append_cflag(builder_module, "-I%s", pwd);
    module__append_lflag(builder_module, "%s", module_file__path(builder_ir));
}

int main(int argc, char** argv) {
    pwd = getenv("PWD");
    assert(pwd);

    module__initialize_builder_module();

    if (argc != 2) {
        fprintf(stderr, "Usage: <builder_bin> <supported_module_name>\n");
        print__supported_modules_names();
        exit(1);
    }

    const char* target_supported_module_path = argv[1];
    supported_module_t* target_supported_module = 0;
    for (uint32_t sm_index = 0; sm_index < ARRAY_SIZE(supported_modules); ++sm_index) {
        supported_module_t* sm = &supported_modules[sm_index];
        if (!strcmp(sm->module_dir_path, target_supported_module_path)) {
            target_supported_module = sm;
            break ;
        }
    }
    if (!target_supported_module) {
        fprintf(stderr, "Unsupported module: %s\n", target_supported_module_path);
        print__supported_modules_names();
        exit(1);
    }
    supported_module__initialize(target_supported_module);

    module__rebuild_forever(target_supported_module->module, "/usr/bin/gcc", supported_module__execute, target_supported_module);
    
    return 0;
}
