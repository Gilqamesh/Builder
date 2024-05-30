#include "client/client.h"
#include "gfx/gfx.h"

int main() {
    gfx__init();

    client_t client = client__create();

    client__run(client);

    client__destroy(client);

    gfx__deinit();

    return 0;
}
