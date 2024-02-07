#include "builder.h"

#include <unistd.h>
#include <stdio.h>

static const char* binary_path  = "example";

static void module__execute(void* user_data) {
    module_t self = (module_t) user_data;

    printf("./%s\n", binary_path);
    if (execl(binary_path, binary_path, 0) == -1) {
        perror(0);
    }
}

int main() {
    const char* c_linker_path = "/usr/bin/gcc";

    module_t example_module = module__create();
    {
        module_file_t example_src_file = module__add_file(example_module, c_linker_path, "example.c");
        module_file_t example_o_file   = module__add_file(example_module, 0, "example.o");
        module_file_t example_bin_file = module__add_file(example_module, 0, "%s", binary_path);

        module_file__add_dependency(example_o_file,  example_src_file);
        module_file__add_dependency(example_bin_file, example_o_file);

        module_file__append_cflag(example_src_file, "-c %s -o %s", module_file__path(example_src_file), module_file__path(example_o_file));

        module__append_lflag(example_module, "%s -o %s", module_file__path(example_o_file), module_file__path(example_bin_file));
    }

    module_t game_module = module__create();
    {
        module_file_t game_src_file = module__add_file(example_module, c_linker_path, "game.c");
        module_file_t game_so_file  = module__add_file(example_module, 0 , "libgame.so");
        module_file_t game_h_file   = module__add_file(example_module, 0, "game.h");

        module_file__add_dependency(game_src_file, game_h_file);
        module_file__add_dependency(game_so_file, game_src_file);

        module_file__append_cflag("%s -o %s -fPIC -shared", module_file__path(game_src_file), module_file__path(game_so_file));
    }

    module__rebuild_forever(example_module, c_linker_path, &module__execute, example_module);

    return 0;
}
