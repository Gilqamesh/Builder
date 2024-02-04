#include "builder.h"

#include <unistd.h>
#include <stdio.h>

static const char* binary_name = "example";

static void module__execute(void* user_data) {
    module_t self = (module_t) user_data;

    printf("./%s\n", binary_name);
    if (execl(binary_name, binary_name, 0) == -1) {
        perror(0);
    }
}

int main() {
    const char* c_linker_path = "/usr/bin/gcc";

    module_t example_module = module__create(0);

    module_file_t example_src_file    = module__add_file(example_module, c_linker_path, "example.c");
    module_file_t example_file_header = module__add_file(example_module, 0, "example.h");
    module_file_t example_ir_file     = module__add_file(example_module, 0, "example.o");
    module_file_t example_bin_file    = module__add_file(example_module, 0, "%s", binary_name);

    module_file__append_cflag(example_src_file, "-c %s -o %s", module_file__path(example_src_file), module_file__path(example_ir_file));

    module__append_lflag(example_module, "%s -o %s", module_file__path(example_ir_file), module_file__path(example_bin_file));

    module_file__add_dependency(example_src_file, example_file_header);
    module_file__add_dependency(example_ir_file,  example_src_file);
    module_file__add_dependency(example_bin_file, example_ir_file);

    module__rebuild_forever(example_module, c_linker_path, &module__execute, example_module);

    return 0;
}
