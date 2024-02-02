#include "builder.h"

int main() {
    module_t example_module = module__create();

    module_file_t example_file = module__add_file(example_module, "/usr/bin/gcc");
    const char* example_module_prefix_dir = "examples/simple";
    module_file__append_cflag(example_file, "-c %s/example.c -o %s/example.o", example_module_prefix_dir, example_module_prefix_dir);
    module__append_lflag(example_module, "%s/example.o -o %s/example", example_module_prefix_dir, example_module_prefix_dir);

    module__compile_with_dependencies(example_module);
    module__wait_for_compilation(example_module);

    int link_result = 0;
    module__link(example_module, "/usr/bin/gcc", &link_result);

    module__destroy(example_module);

    return link_result;
}
