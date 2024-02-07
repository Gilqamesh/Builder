#include "builder.h"

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>

#define ARRAY_SIZE(array) (sizeof(array)/sizeof((array)[0]))

int main(int argc, char** argv) {
    builder__init();

    const char* const supported_files[] = {
        "examples/simple",
        "examples/shared_lib",
        "examples/multiple_compilers",
        "examples/gfx"
    };

    if (argc != 2) {
        fprintf(stderr, "Usage: <builder_bin> <supported_dir>\n");
        goto err_print_supported;
    }

    const char* const target_dir = argv[1];
    const char* found_dir = 0;
    for (uint32_t i = 0; i < ARRAY_SIZE(supported_files); ++i) {
        if (!strcmp(supported_files[i], target_dir)) {
            found_dir = supported_files[i];
            break ;
        }
    }

    if (!found_dir) {
        fprintf(stderr, "Unsupported module: %s\n", target_dir);
        goto err_print_supported;
    }

    const char* bin_name = "build_example";
    obj_t c_compiler        = obj__file("/usr/bin/gcc");
    obj_t o_linker          = obj__file("/usr/bin/gcc");
    obj_t builder_h         = obj__file("builder.h");
    obj_t builder_c         = obj__file("builder.c");
    obj_t builder_o         = obj__file("builder.o");
    obj_t build_example_c   = obj__file("%s/%s.c", found_dir, bin_name);
    obj_t build_example_o   = obj__file("%s/%s.o", found_dir, bin_name);
    obj_t build_example_bin = obj__file("%s/%s", found_dir, bin_name);

    obj_t program =
        obj__process(
            obj__process(
                obj__dependencies(
                    obj__process(
                        obj__dependencies(c_compiler, builder_h, builder_c, 0),
                        builder_o,
                        "%s -g -I. -c %s -o %s -Wall -Wextra -Werror 2>&1 | head -n 25", obj__file_path(c_compiler), obj__file_path(builder_c), obj__file_path(builder_o)
                    ),
                    obj__process(
                        obj__dependencies(c_compiler, builder_h, build_example_c, 0),
                        build_example_o,
                        "%s -g -I. -c %s -o %s -Wall -Wextra -Werror 2>&1 | head -n 25", obj__file_path(c_compiler), obj__file_path(build_example_c), obj__file_path(build_example_o)
                    ),
                    0
                ),
                build_example_bin,
                "%s %s %s -o %s 2>&1 | head -n 25", obj__file_path(o_linker), obj__file_path(builder_o), obj__file_path(build_example_o), obj__file_path(build_example_bin)
            ),
            0,
            "cd %s && ./%s", found_dir, bin_name
        );
    
    obj__print(program);

    while (1) {
        obj__build(program);
        usleep(100000);
    }

    return 0;

err_print_supported:
    fprintf(stderr, "Supported files:\n");
    for (uint32_t i = 0; i < sizeof(supported_files)/sizeof(supported_files[0]); ++i) {
        fprintf(stderr, "    %s\n", supported_files[i]);
    }
    return 1;
}
