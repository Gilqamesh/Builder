#include "builder.h"

#include <assert.h>

static const char* c_compiler_path   = "/usr/bin/gcc";
static const char* cpp_compiler_path = "/usr/bin/g++";
static const char* cpp_linker_path   = "/usr/bin/g++";

static module_t lib_module = 0;
static module_t example_module = 0;

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
    module_file_t lib_file = module__add_file(lib_module, c_compiler_path);
    const char* lib_module_prefix_dir = "examples/multiple_compilers/lib";
    module_file__append_cflag(lib_file, "-c %s/lib.c -o %s/lib.o", lib_module_prefix_dir, lib_module_prefix_dir);
    module__append_lflag(lib_module, "%s/lib.o", lib_module_prefix_dir);
    module__append_cflag(lib_module, "-I%s", lib_module_prefix_dir);

    assert(linked_libs_top < sizeof(linked_libs) / sizeof(linked_libs[0]));
    linked_libs[linked_libs_top++] = "-lm";
}

static void create_example_module() {
    if (example_module) {
        return ;
    }

    example_module = module__create();
    const char* example_module_prefix_dir = "examples/multiple_compilers";
    module_file_t example_c_file = module__add_file(example_module, c_compiler_path);
    module_file__append_cflag(example_c_file, "-c %s/example.c -o %s/example_c.o", example_module_prefix_dir, example_module_prefix_dir);
    module__append_lflag(example_module, "%s/example_c.o", example_module_prefix_dir);

    module_file_t example_cpp_file = module__add_file(example_module, cpp_compiler_path);
    module_file__append_cflag(example_cpp_file, "-c %s/example.cpp -o %s/example_cpp.o", example_module_prefix_dir, example_module_prefix_dir);
    module__append_lflag(example_module, "%s/example_cpp.o", example_module_prefix_dir);
    module__append_lflag(example_module, "-o %s/example", example_module_prefix_dir);

    create_lib_module();
    assert(lib_module);
    module__add_dependency(example_module, lib_module);
}

static void append_linked_libs(module_t module) {
    for (int linked_lib_index = 0; linked_lib_index < linked_libs_top; ++linked_lib_index) {
        module__append_lflag(module, linked_libs[linked_lib_index]);
    }
}

int main() {
    create_example_module();

    append_linked_libs(example_module);

    module__compile_with_dependencies(example_module);
    module__wait_for_compilation(example_module);

    int link_result = 0;
    module__link(example_module, cpp_linker_path, &link_result);

    return link_result;
}
