#include "server.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        LOG("Server", "Usage: <" << argv[0] << "> <shared memory name>");
        return 1;
    }

    using namespace ipc_mem;

    LOG("Server", "initializing shared memory...");
    {
        shared_memory_t shared_memory(argv[1], 1024);
        LOG("Server", "initialized shared memory!");

        while (1) {
            {
                boost::interprocess::scoped_lock guard_mutex_holders(g_shared_memory->m_mutex_holders);

                if (g_shared_memory->m_holders->size() == 1) {
                }
            }
        }

        LOG("Server", "deinitializing shared memory...");
    }
    LOG("Server", "deinitialized shared memory!");

    return 0;
}


/*

    server:
        owns locks
        wait with timeout

*/