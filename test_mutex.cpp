#include "ipc_cpp/mem.h"

#include <boost/process.hpp>
#include <filesystem>

using namespace ipc_mem;

struct shared_object_t {
    process_safe_data_t<int> m_data;
};

int main(int argc, char** argv) {
    if (argc == 1) {
        // LOG("Test Mutex", "Process path: " << std::filesystem::current_path() / argv[0]);
        std::string shared_memory_name("Asdijsdf");
        init(shared_memory_name, 1024);

        std::string shared_object_name("asjdfbgs");
        auto shared_object = malloc_named<shared_object_t>(shared_object_name);

        LOG("Test Mutex", "Spawning child processes...");
        boost::process::group process_group;
        boost::process::spawn(
            std::string(argv[0]) + " " + shared_memory_name + " " + shared_object_name,
            process_group
        );
        LOG("Test Mutex", "Spawned child processes!");

        LOG("Test Mutex", "Reading data...");
        shared_object->m_data.read([](auto& self, auto& data) {
            (void) self;
            LOG("Test Mutex", data);
        });
        LOG("Test Mutex", "Read data!");

        LOG("Test Mutex", "Waiting a bit");
        std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1, 1>>(0.25));
        LOG("Test Mutex", "Waited!");

        LOG("Test Mutex", "Terminating group...");
        process_group.terminate();
        LOG("Test Mutex", "Terminated group!");

        LOG("Test Mutex", "Waiting for group...");
        boost::system::error_code e;
        process_group.wait(e);
        LOG("Test Mutex", "Waited for group!");

        LOG("Test Mutex", "Reading data...");
        shared_object->m_data.read([](auto& self, auto& data) {
            (void) self;
            LOG("Test Mutex", data);
        });
        LOG("Test Mutex", "Read data!");

        free(shared_object);

        deinit();
    } else {
        if (argc != 3) {
            printf("Usage: <%s> <shared memory name> <shared object name>\n", argv[0]);
            return 1;
        }

        init(argv[1]);

        LOG("Test Mutex", "Finding shared object...");
        auto shared_object = find_named<shared_object_t>(argv[2]);
        if (!shared_object) {
            LOG("Test Mutex", "Did not find shared object.");
            deinit();
            return 1;
        }
        LOG("Test Mutex", "Found shared object!");

        LOG("Test Mutex", "Writing to data forever...");
        int new_data = 1;
        while(1) {
            shared_object->m_data.write([new_data](auto& self, auto& data) {
                (void) self;
                data = new_data;
            });
            ++new_data;
        }
        LOG("Test Mutex", "Wrote to data!");

        deinit();
    }

    return 0;
}
