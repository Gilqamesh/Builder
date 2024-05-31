#include "simple.h"

obj_t obj__build_simple() {
    obj_t engine_time      = obj__time();
    obj_t oscillator_400ms = obj__oscillator(engine_time, 400);
    obj_t oscillator_10s   = obj__oscillator(engine_time, 10000);
    obj_t c_compiler       = obj__file_modified(oscillator_10s,   "/usr/bin/gcc");
    obj_t o_linker         = obj__file_modified(oscillator_10s,   "/usr/bin/gcc");
    obj_t example_h        = obj__file_modified(oscillator_400ms, "example.h");
    obj_t example_c        = obj__file_modified(oscillator_400ms, "example.c");
    obj_t example_o = obj__file_modified(
        obj__thread(
            obj__sh(
                obj__list(example_h, example_c, 0),
                0,
                "%s -g -c %s -o example.o -Wall -Wextra -Werror", obj__file_modified_path(c_compiler), obj__file_modified_path(example_c)
            ),
            oscillator_400ms
        ),
        "example.o"
    );
    obj_t example_bin = obj__file_modified(
        obj__thread(
            obj__sh(
                example_o,
                0,
                "%s %s -o example", obj__file_modified_path(o_linker), obj__file_modified_path(example_o)
            ),
            oscillator_400ms
        ),
        "example"
    );

    obj_t program = obj__sh(
        example_bin,
        0,
        "./%s", obj__file_modified_path(example_bin)
    );
    
    return program;
}
