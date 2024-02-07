#include "builder.h"

#include <unistd.h>

int main() {
    builder__init();

    const char* lib_dir = "lib";

    obj_t c_compiler    = obj__file("/usr/bin/gcc");
    obj_t cpp_compiler  = obj__file("/usr/bin/g++");
    obj_t cpp_linker    = obj__file("/usr/bin/g++");
    obj_t example_h     = obj__file("example.h");
    obj_t example_c     = obj__file("example.c");
    obj_t example_o     = obj__file("example.o");
    obj_t example_cpp   = obj__file("example.cpp");
    obj_t example_cpp_o = obj__file("example.cpp.o");
    obj_t lib_h         = obj__file("%s/lib.h", lib_dir);
    obj_t lib_c         = obj__file("%s/lib.c", lib_dir);
    obj_t lib_o         = obj__file("%s/lib.o", lib_dir);
    obj_t example_bin   = obj__file("example");


    obj_t program =
        obj__process(
            obj__process(
                obj__dependencies(
                    obj__process(
                        obj__dependencies(c_compiler, example_h, example_c, 0),
                        example_o,
                        "%s -I%s -g -c %s -o %s -Wall -Wextra -Werror 2>&1 | head -n 25", obj__file_path(c_compiler), lib_dir, obj__file_path(example_c), obj__file_path(example_o)
                    ),
                    obj__process(
                        obj__dependencies(cpp_compiler, lib_h, example_cpp, example_h, 0),
                        example_cpp_o,
                        "%s -I%s -g -c %s -o %s -Wall -Wextra -Werror 2>&1 | head -n 25", obj__file_path(cpp_compiler), lib_dir, obj__file_path(example_cpp), obj__file_path(example_cpp_o)
                    ),
                    obj__process(
                        obj__dependencies(c_compiler, lib_h, lib_c, 0),
                        lib_o,
                        "%s -g -c %s -o %s -Wall -Wextra -Werror 2>&1 | head -n 25", obj__file_path(c_compiler), obj__file_path(lib_c), obj__file_path(lib_o)
                    ),
                    cpp_linker,
                    0
                ),
                example_bin,
                "%s %s %s %s -o %s | head -n 25", obj__file_path(cpp_linker), obj__file_path(lib_o), obj__file_path(example_o), obj__file_path(example_cpp_o), obj__file_path(example_bin)
            ),
            0,
            "./%s", obj__file_path(example_bin)
        );

    obj__print(program);

    while (1) {
        obj__build(program);
        usleep(250000);
    }

    return 0;
}
