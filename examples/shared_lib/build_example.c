#include "builder.h"

#include <unistd.h>

int main() {
    module_t example_module = module__create();

    const char* example_module_prefix_dir = "examples/shared_lib";
    module_file_t example_file = module__add_file(example_module, "/usr/bin/gcc", "%s/example.c", example_module_prefix_dir);
    module_file__append_cflag(example_file, "-c %s/example.c -o %s/example.o -g", example_module_prefix_dir, example_module_prefix_dir);
    module__append_lflag(example_module, "%s/example.o -o %s/example", example_module_prefix_dir, example_module_prefix_dir);

    module_t shared_lib_module = module__create();
    module_file_t shared_lib_file_c = module__add_file(shared_lib_module, "/usr/bin/gcc", "%s/shared_lib.c", example_module_prefix_dir);
    module_file__append_cflag(shared_lib_file_c, "%s/shared_lib.c -o %s/libshared_lib.so -fPIC -shared", example_module_prefix_dir, example_module_prefix_dir, example_module_prefix_dir);

    module_file_t shared_lib_file_h = module__add_file(shared_lib_module, 0, "%s/shared_lib.h", example_module_prefix_dir);
    module_file__add_dependency(shared_lib_file_c, shared_lib_file_h);

    while (1) {
        module__compile_with_dependencies(example_module);
        module__wait_for_compilation(example_module);
        module__link(example_module, "/usr/bin/gcc", 0);

        module__compile_with_dependencies(shared_lib_module);
        module__wait_for_compilation(shared_lib_module);
        
        usleep(100000);
    }
}
