#include "../builder.h"

int main() {
    builder__init();

    obj_t t = obj__time();
    obj_t compiler = obj__file_modified(t, "/usr/bin/gcc");
    obj_t client_c = obj__file_modified(t, "client.c");
    obj_t client_h = obj__file_modified(t, "client.h");
    obj_t client_o = obj__file_modified(t, "client.o");

    obj__sh(
        obj__list(client_h, client_c, 0),
        client_o,
        0,
        0,
        "%s -g -c %s -o %s -Wall -Wextra -Werror", obj__file_modified_path(compiler), obj__file_modified_path(client_c), obj__file_modified_path(client_o)
    );

    return obj__run(t);
}
