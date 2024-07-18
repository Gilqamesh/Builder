#include "test_msg.h"

#include <iostream>
#include <cassert>

#include <boost/process.hpp>

int main(int argc, char** argv) {
    using namespace boost::interprocess;
    using namespace boost::process;

    if (argc != 3) {
        std::cerr << "usage: <" << argv[0] << "> <shared_memory_name> <named memory>" << std::endl;
        return 1;
    }

    shared_memory_t shared_memory(argv[1]);

    offset_ptr_t<shared_t> shared = shared_memory.find_named<shared_t>(argv[2]);
    if (!shared) {
        throw std::runtime_error("could not find");
    }

    shared->m_messages.push(
        shared_memory.malloc<message_t<custom_message_t>>(
            &shared_memory,
            custom_message_t::MESSAGE_RPC,
            rpc_t{
                .fn = shared->fn,
                .param = 42
            }
        )
    );

    return 0;
}
