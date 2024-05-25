#include "builder.h"
#include "builder_gfx.h"

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
        "examples/multiple_compilers"
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
    obj_t engine_time       = obj__time();
    obj_t oscillator_200ms  = obj__oscillator(engine_time, 200);
    obj_t oscillator_10s    = obj__oscillator(engine_time, 10000);
    obj_t c_compiler        = obj__file_modified(oscillator_10s,   "/usr/bin/gcc");
    obj_t o_linker          = obj__file_modified(oscillator_10s,   "/usr/bin/gcc");
    obj_t builder_h         = obj__file_modified(oscillator_200ms, "builder.h");
    obj_t builder_c         = obj__file_modified(oscillator_200ms, "builder.c");
    obj_t builder_o         = obj__file_modified(oscillator_200ms, "builder.o");
    obj_t build_example_c   = obj__file_modified(oscillator_200ms, "%s/%s.c", found_dir, bin_name);
    obj_t build_example_o   = obj__file_modified(oscillator_200ms, "%s/%s.o", found_dir, bin_name);
    obj_t build_example_bin = obj__file_modified(oscillator_200ms, "%s/%s",   found_dir, bin_name);
    obj_t builder_gfx_h     = obj__file_modified(oscillator_200ms, "builder_gfx.h");
    obj_t builder_gfx_c     = obj__file_modified(oscillator_200ms, "builder_gfx.c");
    obj_t builder_gfx_o     = obj__file_modified(oscillator_200ms, "builder_gfx.o");

    obj__sh(
        obj__sh(
            obj__list(
                obj__sh(
                    obj__list(c_compiler, builder_h, builder_c, 0),
                    builder_o,
                    0,
                    0,
                    "%s -g -I. -c %s -o %s -Wall -Wextra -Werror", obj__file_modified_path(c_compiler), obj__file_modified_path(builder_c), obj__file_modified_path(builder_o)
                ),
                obj__sh(
                    obj__list(c_compiler, builder_h, build_example_c, 0),
                    build_example_o,
                    0,
                    0,
                    "%s -g -I. -c %s -o %s -Wall -Wextra -Werror", obj__file_modified_path(c_compiler), obj__file_modified_path(build_example_c), obj__file_modified_path(build_example_o)
                ),
                obj__sh(
                    obj__list(c_compiler, builder_h, builder_gfx_h, builder_gfx_c, 0),
                    builder_gfx_o,
                    0,
                    0,
                    "%s -g -I. -c %s -o %s -Wall -Wextra -Werror", obj__file_modified_path(c_compiler), obj__file_modified_path(builder_gfx_c), obj__file_modified_path(builder_gfx_o)
                ),
                0
            ),
            build_example_bin,
            0,
            0,
            "%s %s %s %s -o %s libraylib.a -lm", obj__file_modified_path(o_linker), obj__file_modified_path(builder_o), obj__file_modified_path(build_example_o), obj__file_modified_path(builder_gfx_o), obj__file_modified_path(build_example_bin)
        ),
        0,
        engine_time,
        0,
        "cd %s && ./%s", found_dir, bin_name
    );
    
    // while (1) {
    //     obj__run(engine_time);
    //     usleep(10000);
    // }
    builder_gfx__exec(engine_time);

    return 0;

err_print_supported:
    fprintf(stderr, "Supported files:\n");
    for (uint32_t i = 0; i < sizeof(supported_files)/sizeof(supported_files[0]); ++i) {
        fprintf(stderr, "    %s\n", supported_files[i]);
    }
    return 1;
}
