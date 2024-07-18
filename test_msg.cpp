#include "test_msg.h"

#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

#include <boost/process.hpp>

static void dispatch(offset_ptr_t<shared_t> shared, offset_ptr_t<message_t<custom_message_t>> msg) {
    switch (msg->m_message_header.m_id) {
    case custom_message_t::MESSAGE_RPC: {
        rpc_t rpc;
        (*msg) >> rpc;
        rpc.fn(rpc.param);
    } break ;
    default: {
        std::cerr << "dispatch is not implemented for message type " << (int) msg->m_message_header.m_id << std::endl;
    }
    }
}

int main() {
    using namespace boost::interprocess;
    using namespace boost::process;

    shared_memory_t shared_memory("asdbahsf", 2 * 1024);

    std::string shared_memory_name = "shared";
    offset_ptr_t<shared_t> shared = shared_memory.malloc_named<shared_t>(shared_memory_name, &shared_memory);

    shared->fn = [](int a) {
        std::cout << "Hello from lambda: " << a << std::endl;
    };

    bool should_queue_thread_run = true;
    std::thread queue_thread([shared, &shared_memory, &should_queue_thread_run](){
        while (should_queue_thread_run) {
            while (!shared->m_messages.empty()) {
                offset_ptr_t<message_t<custom_message_t>> message = shared->m_messages.pop();
                dispatch(shared, message);
                shared_memory.free(message);
            }
            std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(0.1));
        }
    });

    child c("test_msg2 " + shared_memory.m_shared_memory_name + " " + shared_memory_name.c_str());
    c.wait();

    while (!shared->m_messages.empty()) {
        std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(0.5));
    }
    should_queue_thread_run = false;
    queue_thread.join();
    assert(shared->m_messages.empty());
    shared_memory.free(shared);

    return 0;
}
