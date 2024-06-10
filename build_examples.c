#include "builder.h"
#include "builder_gfx.h"

// #include "examples/simple/simple.h"

#include <unistd.h>

int main() {
    if (builder__init()) {
        return 1;
    }

    // obj_t greet = obj__greet();

    obj__run(obj__present());
    while (1) {
        obj__run(_->engine_time);
        usleep(2000);
    }

    builder__deinit();

    return 0;
}
