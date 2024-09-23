#include "mem.h"
#include "mutex.h"
#include "connection.h"

#include <boost/process.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

#include <iostream>
#include <list>
#include <queue>
#include <cassert>
#include <unordered_set>
#include <unordered_map>

#include <x86intrin.h>

#if 1
#define MUTEX_NET
#endif

using namespace ipc_mem;

struct shared_obj_t {
    size_t m_data;

#if defined(MUTEX_NET)
    mutex_client_t m_mutex_data;
#else
    boost::interprocess::interprocess_mutex m_mutex_data;
#endif
};

static int rand_inclusive(int start, int end) {
    assert(start < end);
    return start + std::rand() % (end - start + 1);
}

static double rand_inclusive(double start, double end) {
    assert(start < end);
    return start + (rand() / (RAND_MAX / (end - start)));
}

static void test_mutex(offset_ptr_t<shared_obj_t> shared_obj, connection_t& connection) {
    size_t n_of_operations = 50;
    for (size_t operation_index = 0; operation_index < n_of_operations; ++n_of_operations) {
        size_t operation_type = rand_inclusive(0, 5);
        if (operation_type == 0) {
            auto guard_write = shared_obj->m_mutex_data.scoped_write(&connection);
            auto guard_read = guard_write.scoped_read();
            auto guard_write2 = guard_write.scoped_write();
            LOG("Client", "Write start: " << shared_obj->m_data);
            size_t n_runs = rand_inclusive(10000, 100000);
            for (size_t i = 0; i < n_runs; ++i) {
                ++shared_obj->m_data;
            }
            LOG("Client", "Write done: " << shared_obj->m_data);
        } else {
            auto guard_read = shared_obj->m_mutex_data.scoped_read(&connection);
            LOG("Client", "Read start: " << shared_obj->m_data);
            std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(rand_inclusive(0.01, 0.1)));
            LOG("Client", "Read done: " << shared_obj->m_data);
        }
    }

#if defined(MUTEX_NET)
    // for (size_t i = 0; i < 1000000; ++i) {
    //     auto guard_write = shared_obj->m_mutex_data.scoped_write(&connection);

    //     // shared_obj->m_mutex_data.write_lock(&connection);

    //     {
    //         auto guard_read = shared_obj->m_mutex_data.scoped_read(&connection);
    //         auto guard_read2 = shared_obj->m_mutex_data.scoped_read(&connection);
    //         auto guard_read3 = shared_obj->m_mutex_data.scoped_read(&connection);
    //         auto guard_read4 = shared_obj->m_mutex_data.scoped_read(&connection);
    //         auto guard_read5 = shared_obj->m_mutex_data.scoped_read(&connection);
    //     }
    //     // shared_obj->m_mutex_data.read_lock(&connection);
    //     // shared_obj->m_mutex_data.read_unlock(&connection);

    //     ++shared_obj->m_data;
        
    //     // shared_obj->m_mutex_data.write_unlock(&connection);
    // }
#else
    for (size_t i = 0; i < 1000000; ++i) {
        shared_obj->m_mutex_data.lock();

        ++shared_obj->m_data;

        shared_obj->m_mutex_data.unlock();
    }
#endif
}

static int client_main(int argc, char** argv) {    
    if (argc != 5) {
        LOG("Client", "Usage: <" << argv[0] << "> <shared memory name> <shared object name> <host> <service_name>");
        return 1;
    }

    size_t t_begin = __rdtsc();

    LOG("Client", "Connecting to socket...");    
    boost::system::error_code error_code;
    boost::asio::io_context io_context;
    // boost::asio::ip::tcp::resolver resolver(io_context);
    // boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(argv[3], argv[4]);

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1", error_code), 8080);

    connection_t connection(io_context);
    connection.connect(endpoint, error_code);
    LOG("Client", "Connected to socket!");


    if (connection.is_open()) {
        LOG("Client", "Socket is open");

        LOG("Client", "Joining shared memory...");
        init(argv[1]);
        LOG("Client", "Joined shared memory!");

        LOG("Client", "Finding shared object...");
        auto shared_obj = find_named<shared_obj_t>(argv[2]);
        LOG("Client", "Found shared object!");

        LOG("Client", "Writing to object...");
        test_mutex(shared_obj, connection);
        LOG("Client", "Wrote to object!");

        LOG("Client", "Deinitializing memory...");
        deinit();
        LOG("Client", "Deinitialized memory!");
    }

    size_t t_end = __rdtsc();
    LOG("Client", "Time taken: " << static_cast<double>(t_end - t_begin) / 1000000.0 << "MCy");

    return 0;
}

int main(int argc, char** argv) {
    srand(42);

    if (argc == 5) {
        return client_main(argc, argv);
    }

    size_t t_begin = __rdtsc();

    LOG("Server", "Creating endpoint...");    
    boost::system::error_code error_code;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 8080));
    LOG("Server", "Created endpoint!");

    LOG("Server", "Initializing shared memory...");
    std::string shared_memory_name = "aldadsf8s";
    init(shared_memory_name, 2 * 1024);
    LOG("Server", "Initialized shared memory!");

    LOG("Server", "Creating shared object...");
    std::string shared_object_name = "hadsg038";
    auto shared_obj = malloc_named<shared_obj_t>(shared_object_name);
    LOG("Server", "Created shared object!");

    LOG("Server", "Creating child processes...");
    boost::process::group group;
    size_t n_of_connections = 2;
    struct server_connection_t : public connection_t{
        boost::process::child m_child;
    };
    std::list<server_connection_t> server_connections;
    for (size_t i = 0; i < n_of_connections; ++i) {
        boost::process::child child(
            std::string(argv[0]) + " " + shared_memory_name + " " + shared_object_name + " 127.0.0.1 mutex_service",
            group
        );
        server_connection_t server_connection(io_context);
        server_connection.m_child = std::move(child);
        acceptor.accept(server_connection);
        server_connections.emplace_back(std::move(server_connection));
    }
    LOG("Server", "Created child processes!");

    LOG("Server", "Waiting for group...");
    // const auto time_out = std::chrono::duration<double, std::ratio<1, 1>>(0.01);
    std::unordered_map<size_t, mutex_server_t*> shared_locks;
    while (!server_connections.empty()) {
        for (auto& server_connection : server_connections) {
            server_connection.receive(shared_locks);
        }

        auto server_connection_it = server_connections.begin();
        while (server_connection_it != server_connections.end()) {
            if (!server_connection_it->m_child.running(error_code)) {
                // todo: do some unwinding if the connection didn't give back shared resources
                LOG("Server", "Connection ended, total received bytes: " << static_cast<double>(server_connection_it->m_debug_received_bytes) / 1024.0 / 1024.0 << "MB, total sent bytes: " << static_cast<double>(server_connection_it->m_debug_sent_bytes) / 1024.0 / 1024.0 << "MB");
                server_connection_it = server_connections.erase(server_connection_it);
            } else {
                ++server_connection_it;
            }
        }
    }
    LOG("Server", "Waited for group!");

    LOG("Server", "Reading object...");
    LOG("Server", shared_obj->m_data);
    LOG("Server", "Read object!");

    LOG("Server", "Freeing shared object...");
    free(shared_obj);
    LOG("Server", "Freed shared object!");

    LOG("Server", "Deinitializing memory...");
    deinit();
    LOG("Server", "Deinitialized memory!");

    size_t t_end = __rdtsc();
    LOG("Server", "Time taken: " << static_cast<double>(t_end - t_begin) / 1000000.0 << "MCy");

    return 0;
}
