#include "builder.h"

#include <unistd.h>

int main() {
    builder__init();

    obj_t c_compiler  = obj__file("/usr/bin/gcc");
    obj_t o_linker    = obj__file("/usr/bin/gcc");
    obj_t example_h   = obj__file("example.h");
    obj_t example_c   = obj__file("example.c");
    obj_t example_o   = obj__file("example.o");
    obj_t example_bin = obj__file("example");

    obj_t program =
        obj__process(
            obj__process(
                obj__process(
                    obj__dependencies(example_h, example_c, 0),
                    example_o,
                    "%s -g -c %s -o %s -Wall -Wextra -Werror 2>&1 | head -n 25", obj__file_path(c_compiler), obj__file_path(example_c), obj__file_path(example_o)
                ),
                example_bin,
                "%s %s -o %s 2>&1 | head -n 25", obj__file_path(o_linker), obj__file_path(example_o), obj__file_path(example_bin)
            ),
            0,
            "./%s", obj__file_path(example_bin)
        );

    obj__print(program);

    while (1) {
        obj__build(program);
        usleep(100000);
    }

    return 0;
}
