#include "../builder.h"

int main() {
    builder__init();

    obj_t t = obj__time();
    obj_t compiler = obj__file_modified(t, "/usr/bin/gcc");
    obj_t gfx_c = obj__file_modified(t, "gfx.c");
    obj_t gfx_h = obj__file_modified(t, "gfx.h");
    obj_t gfx_o = obj__file_modified(t, "gfx.o");
    
    obj__sh(
        obj__list(gfx_h, gfx_c, 0),
        gfx_o,
        0,
        0,
        "%s -g -c %s -o %s -Wall -Wextra -Werror", obj__file_modified_path(compiler), obj__file_modified_path(gfx_c), obj__file_modified_path(gfx_o)
    );

    return obj__run(t);
}
