#include "builder.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

static const char* c_compiler_path   = "/usr/bin/gcc";
static const char* cpp_compiler_path = "/usr/bin/g++";
static const char* cpp_linker_path   = "/usr/bin/g++";

static module_t lib_module = 0;
static module_t example_module = 0;
static const char* binary_name = "example";

static void create_lib_module();
static void create_example_module();
static void append_linked_libs(module_t module);

static int linked_libs_top = 0;
static char* linked_libs[16];

static void create_lib_module() {
    if (lib_module) {
        return ;
    }

    lib_module = module__create();
    const char* lib_module_prefix_dir = "lib";
    module_file_t lib_src_file = module__add_file(lib_module, c_compiler_path, "%s/lib.c", lib_module_prefix_dir);
    module_file_t lib_o_file   = module__add_file(lib_module, 0, "%s/lib.o", lib_module_prefix_dir);
    module_file_t lib_h_file   = module__add_file(lib_module, 0, "%s/lib.h", lib_module_prefix_dir);

    module_file__add_dependency(lib_src_file, lib_h_file);
    module_file__add_dependency(lib_o_file, lib_src_file);

    module_file__append_cflag(lib_src_file, "-c %s -o %s", module_file__path(lib_src_file), module_file__path(lib_o_file));

    module__append_cflag(lib_module, "-I%s", lib_module_prefix_dir);
    module__append_lflag(lib_module, "%s", module_file__path(lib_o_file));

    assert(linked_libs_top < sizeof(linked_libs) / sizeof(linked_libs[0]));
    linked_libs[linked_libs_top++] = "-lm";
}

static void create_example_module() {
    if (example_module) {
        return ;
    }

    example_module = module__create();
    module_file_t example_c_file     = module__add_file(example_module, c_compiler_path, "example.c");
    module_file_t example_o_file     = module__add_file(example_module, 0, "example.c.o");
    module_file_t example_h_file     = module__add_file(example_module, 0, "example.c.o");
    module_file_t example_cpp_file   = module__add_file(example_module, cpp_compiler_path, "example.cpp");
    module_file_t example_cpp_o_file = module__add_file(example_module, 0, "example.cpp.o");

    module_file__add_dependency(example_o_file, example_c_file);
    module_file__add_dependency(example_c_file, example_h_file);
    module_file__add_dependency(example_cpp_file, example_h_file);
    module_file__add_dependency(example_cpp_o_file, example_cpp_file);

    module_file__append_cflag(example_c_file, "-c %s -o %s", module_file__path(example_c_file), module_file__path(example_o_file));
    module_file__append_cflag(example_cpp_file, "-c %s -o %s", module_file__path(example_cpp_file), module_file__path(example_cpp_o_file));

    module__append_lflag(example_module, "%s %s -o %s", module_file__path(example_o_file), module_file__path(example_cpp_o_file), binary_name);

    create_lib_module();
    assert(lib_module);
    module__add_dependency(example_module, lib_module);
}

static void append_linked_libs(module_t module) {
    for (int linked_lib_index = 0; linked_lib_index < linked_libs_top; ++linked_lib_index) {
        module__append_lflag(module, linked_libs[linked_lib_index]);
    }
}

static void module__execute(void* user_data) {
    module_t self = (module_t) user_data;

    printf("./%s\n", binary_name);
    if (execl(binary_name, binary_name, 0) == -1) {
        perror(0);
    }
}

int main() {
    create_example_module();

    append_linked_libs(example_module);

    module__rebuild_forever(example_module, cpp_linker_path, &module__execute, example_module);

    return 0;
}
