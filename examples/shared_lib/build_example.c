#include "builder.h"

#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static const char* c_compiler_path = "/usr/bin/gcc";

static const char* binary_name = "example";

int main() {
    module_t shared_lib_module = module__create();
    module_file_t shared_lib_c_file  = module__add_file(shared_lib_module, c_compiler_path, "shared_lib.c");
    module_file_t shared_lib_h_file  = module__add_file(shared_lib_module, 0, "shared_lib.h");
    module_file_t shared_lib_so_file = module__add_file(shared_lib_module, 0, "libshared_lib.so");

    module_file__add_dependency(shared_lib_c_file, shared_lib_h_file);
    module_file__add_dependency(shared_lib_so_file, shared_lib_c_file);

    module_file__append_cflag(shared_lib_c_file, "%s -o %s -fPIC -shared -g", module_file__path(shared_lib_c_file), module_file__path(shared_lib_so_file));

    module_t example_module = module__create();

    module_file_t example_c_file = module__add_file(example_module, c_compiler_path, "example.c");
    module_file_t example_o_file = module__add_file(example_module, 0, "example.o");

    module_file__add_dependency(example_o_file, example_c_file);

    module_file__append_cflag(example_c_file, "-c %s -o %s -g", module_file__path(example_c_file), module_file__path(example_o_file));

    module__append_lflag(example_module, "%s -o %s", module_file__path(example_o_file), binary_name);

    module__add_dependency(example_module, shared_lib_module);

    module__rebuild_forever(example_module, c_compiler_path, 0, 0);

    return 0;
}
