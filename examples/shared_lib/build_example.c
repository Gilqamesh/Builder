#include "builder.h"

#include <stdio.h>
#include <unistd.h>

int main() {
    builder__init();

    obj_t c_compiler    = obj__file_time("/usr/bin/gcc");
    obj_t c_linker      = obj__file_time("/usr/bin/gcc");
    obj_t example_c     = obj__file_time("example.c");
    obj_t example_o     = obj__file_time("example.o");
    obj_t shared_lib_h  = obj__file_time("shared_lib.h");
    obj_t shared_lib_c  = obj__file_time("shared_lib.c");
    obj_t shared_lib_so = obj__file_time("libshared_lib.so");
    obj_t example_bin   = obj__file_time("example");

    obj_t shared_lib_program =
        obj__sh(
            obj__list(c_linker, shared_lib_h, shared_lib_c, 0),
            shared_lib_so,
            "%s -g %s -o %s -fPIC -shared -Wall -Wextra -Werror 2>&1 | head -n 25", obj__file_time_path(c_linker), obj__file_time_path(shared_lib_c), obj__file_time_path(shared_lib_so)
        );

    obj_t program =
        obj__sh(
            obj__sh(
                obj__list(
                    c_linker,
                    obj__sh(
                        obj__list(c_compiler, shared_lib_h, example_c, 0),
                        example_o,
                        "%s -g -c %s -o %s -Wall -Wextra -Werror 2>&1 | head -n 25", obj__file_time_path(c_compiler), obj__file_time_path(example_c), obj__file_time_path(example_o)
                    ),
                    0
                ),
                example_bin,
                "%s %s -o %s | head -n 25", obj__file_time_path(c_linker), obj__file_time_path(example_o), obj__file_time_path(example_bin)
            ),
            0,
            "./%s", obj__file_time_path(example_bin)
        );

    obj__print(shared_lib_program);
    obj__print(program);

    while (1) {
        obj__build(shared_lib_program);
        obj__build(program);
        usleep(50000);
    }

    return 0;
}
