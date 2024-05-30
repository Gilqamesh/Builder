#include "builder.h"
#include "builder_gfx.h"

#include <unistd.h>

int main() {
    builder__init();

    obj_t engine_time      = obj__time();
    obj_t oscillator_400ms = obj__oscillator(engine_time, 400);
    obj_t oscillator_10s   = obj__oscillator(engine_time, 10000);
    obj_t c_compiler       = obj__file_modified(oscillator_10s,   "/usr/bin/gcc");
    obj_t o_linker         = obj__file_modified(oscillator_10s,   "/usr/bin/gcc");
    obj_t example_h        = obj__file_modified(oscillator_400ms, "example.h");
    obj_t example_c        = obj__file_modified(oscillator_400ms, "example.c");
    obj_t example_o = obj__file_modified(
        obj__wait(
            obj__list(
                obj__sh(
                    obj__list(example_h, example_c, 0),
                    0,
                    "%s -g -c %s -o example.o -Wall -Wextra -Werror", obj__file_modified_path(c_compiler), obj__file_modified_path(example_c)
                ),
                oscillator_400ms,
                0
            ),
            0
        ),
        "example.o"
    );
    obj_t example_bin = obj__file_modified(
        obj__wait(
            obj__list(
                obj__sh(
                    example_o,
                    0,
                    "%s %s -o example", obj__file_modified_path(o_linker), obj__file_modified_path(example_o)
                ),
                oscillator_400ms,
                0
            ),
            0
        ),
        "example"
    );

    obj_t program = obj__sh(
        example_bin,
        0,
        "./%s", obj__file_modified_path(example_bin)
    );
    
    obj__wait(
        program,
        0
    );

    builder_gfx__exec(engine_time, program);

    return 0;
}
