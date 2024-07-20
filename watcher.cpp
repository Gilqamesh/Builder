#include "ipc_cpp/mem.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        LOG("Watcher", "Usage: <" << argv[0] << "> <shared memory name>");
        return 1;
    }

    using namespace ipc_mem;

    LOG("Watcher", "initializing shared memory...");
    init(argv[1]);
    LOG("Watcher", "initialized shared memory!");

    while (exists()) {
    }

    LOG("Watcher", "deinitializing shared memory");
    deinit();
    LOG("Watcher", "deinitialized shared memory");

    return 0;
}
