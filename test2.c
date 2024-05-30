#include "builder.h"
#include "builder_gfx.h"

int main() {
    builder__init();

    obj_t t                  = obj__time();
    obj_t osc_10s            = obj__oscillator(t, 10000);
    obj_t osc_200ms          = obj__oscillator(t, 200);
    const char* builder_name = "builder";
    obj_t compiler           = obj__file_modified(osc_10s,   "/usr/bin/gcc");
    obj_t linker             = obj__file_modified(osc_10s,   "/usr/bin/gcc");
    obj_t builder_c          = obj__file_modified(osc_200ms, "builder.c");
    obj_t builder_h          = obj__file_modified(osc_200ms, "builder.h");
    obj_t builder_o          = obj__file_modified(osc_200ms, "builder.o");
    obj_t gfx_builder_c      = obj__file_modified(osc_200ms, "gfx/builder.c");
    obj_t gfx_builder_o      = obj__file_modified(osc_200ms, "gfx/builder.o");
    obj_t gfx_builder_bin    = obj__file_modified(osc_200ms, "gfx/%s", builder_name);
    obj_t client_builder_c   = obj__file_modified(osc_200ms, "client/builder.c");
    obj_t client_builder_o   = obj__file_modified(osc_200ms, "client/builder.o");
    obj_t client_builder_bin = obj__file_modified(osc_200ms, "client/%s", builder_name);
    obj_t user_c             = obj__file_modified(osc_200ms, "user.c");
    obj_t user_o             = obj__file_modified(osc_200ms, "user.o");
    obj_t user_bin           = obj__file_modified(osc_200ms, "user");
    obj_t gfx                = obj__file_modified(osc_200ms, "gfx/gfx.o");
    obj_t client             = obj__file_modified(osc_200ms, "client/client.o");
    obj_t builder_lib = obj__sh(
        obj__list(builder_h, builder_c, 0),
        builder_o,
        0,
        0,
        "%s -g -c %s -o %s -Wall -Wextra -Werror", obj__file_modified_path(compiler), obj__file_modified_path(builder_c), obj__file_modified_path(builder_o)
    );
    obj_t gfx_lib = obj__sh(
        obj__sh(
            obj__list(
                obj__sh(
                    gfx_builder_c,
                    gfx_builder_o,
                    0,
                    0,
                    "%s -g -c %s -o %s -Wall -Wextra -Werror", obj__file_modified_path(compiler), obj__file_modified_path(gfx_builder_c), obj__file_modified_path(gfx_builder_o)
                ),
                builder_lib,
                0
            ),
            gfx_builder_bin,
            0,
            0,
            "%s -o %s %s %s -lm", obj__file_modified_path(linker), obj__file_modified_path(gfx_builder_bin), obj__file_modified_path(gfx_builder_o), obj__file_modified_path(builder_lib)
        ),
        gfx,
        0,
        0,
        "cd gfx && ./%s", builder_name
    );
    obj_t client_lib = obj__sh(
        obj__sh(
            obj__list(
                obj__sh(
                    client_builder_c,
                    client_builder_o,
                    0,
                    0,
                    "%s -g -c %s -o %s -Wall -Wextra -Werror", obj__file_modified_path(compiler), obj__file_modified_path(client_builder_c), obj__file_modified_path(client_builder_o)
                ),
                builder_lib,
                0
            ),
            client_builder_bin,
            0,
            0,
            "%s -o %s %s %s -lm", obj__file_modified_path(linker), obj__file_modified_path(client_builder_bin), obj__file_modified_path(client_builder_o), obj__file_modified_path(builder_lib)
        ),
        client,
        0,
        0,
        "cd client && ./%s", builder_name
    );
    obj_t user_bin_exec = obj__sh(
        obj__sh(
            obj__list(
                gfx_lib,
                client_lib,
                    obj__sh(
                    user_c,
                    user_o,
                    0,
                    0,
                    "%s -g -c %s -o %s -Wall -Wextra -Werror", obj__file_modified_path(compiler), obj__file_modified_path(user_c), obj__file_modified_path(user_o)
                ),
                0
            ),
            user_bin,
            0,
            0,
            "%s %s %s %s -o %s", obj__file_modified_path(linker), obj__file_modified_path(gfx_lib), obj__file_modified_path(client_lib), obj__file_modified_path(user_o), obj__file_modified_path(user_bin)
        ),
        0,
        0,
        0,
        "./%s", obj__file_modified_path(user_bin)
    );

    builder_gfx__exec(t, user_bin_exec);

    return 0;
}
