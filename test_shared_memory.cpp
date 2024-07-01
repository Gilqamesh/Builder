#include <iostream>
#include <thread>
#include <string>
#include <boost/process.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/string.hpp>

using namespace boost::interprocess;

struct obj_t {
    using string_allocator = allocator<char, managed_shared_memory::segment_manager>;
    using shared_string = basic_string<char, std::char_traits<char>, string_allocator>;

    int i;
    shared_string* s;
};

int main(int argc, char** argv) {
    
    if (argc == 1) {
        std::string shared_memory_name = "shared_memory";
        struct shared_memory {
            shared_memory(const std::string& shared_memory_name): m_shared_memory_name(shared_memory_name) {
                shared_memory_object::remove(m_shared_memory_name.c_str());
            }

            ~shared_memory() {
                shared_memory_object::remove(m_shared_memory_name.c_str());
            }

            std::string m_shared_memory_name;
        } shared_memory_guard(shared_memory_name);

        managed_shared_memory shared_memory(create_only, shared_memory_name.c_str(), 1024, 0, read_write);

        std::string segment_name = "shared_segment_name";
        obj_t* obj = shared_memory.construct<obj_t>(segment_name.c_str())();
        obj->s = shared_memory.construct<obj_t::shared_string>("str name")(shared_memory.get_segment_manager());
        std::cout << "obj before running child " << obj->i << ", " << obj->s << std::endl;

        std::cout << "Spawning child process.." << std::endl;
        boost::process::child c("a.out " + shared_memory_name + " " + segment_name);

        std::cout << "Waiting for child in main process.." << std::endl;
        c.wait();

        int exit_code = c.exit_code();
        std::cout << "Child exited with status code: " << exit_code << std::endl;

        std::cout << "obj after running child " << obj->i << ", " << obj->s << std::endl;
    } else {
        std::cout << "Hello from child process" << std::endl;

        const char* named_memory = argv[1];
        const char* segment_name = argv[2];

        managed_shared_memory shared_memory(open_only, named_memory);

        std::pair<obj_t*, managed_shared_memory::size_type> segment = shared_memory.find<obj_t>(segment_name);
        if (segment.second != 1) {
            std::cerr << "unexpected segment size: " << segment.second << ", expected: " << 1 << std::endl;
            return 1;
        }

        obj_t* obj = segment.first;
        std::cout << "writiting to obj->i.." << std::endl;
        obj->i = 3;
        std::cout << "written to obj->i" << std::endl;

        std::cout << "writing to obj->s.." << std::endl;
        *obj->s = "hello from child";
        std::cout << "written to obj->s" << std::endl;
    }

    return 0;
}
