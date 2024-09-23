#include "mem.h"

#include <stdexcept>
#include <fstream>

namespace ipc_mem {
    shared_memory_t* g_shared_memory = 0;

    // void create_server() {
    //     LOG("Mem", "creating server...");
    //     boost::process::spawn(
    //         boost::process::args = { "server", shared_memory_name }
    //     );
    //     LOG("Mem", "created server!");
    // }

    struct address_t {
        struct network_address_t* network_address;
        decltype(boost::this_process::get_id()) m_process_id;
        decltype(std::this_thread::get_id()) m_thread_id;
        std::string process_name;
        std::string shared_memory_name;
    };

    shared_memory_t::shared_memory_t(const std::string& shared_memory_name, size_t shared_memory_size) {
        m_owner = true;
        m_shared_memory_name = shared_memory_name;

        using namespace boost::interprocess;

        shared_memory_object::remove(m_shared_memory_name.c_str());
        m_managed_shared_memory = managed_shared_memory(create_only, m_shared_memory_name.c_str(), shared_memory_size);

        if (m_managed_shared_memory.get_size() < shared_memory_size) {
            throw std::runtime_error("got less memory than expected");
        }
        std::cout << "shared_memory memory left: " << m_managed_shared_memory.get_size() << std::endl;

        // m_holders = malloc_named<decltype(m_holders)::value_type>(std::hash<std::string>()(m_shared_memory_name) + "_holders");
        // std::string file_lock_mutex_holders_name(std::hash<std::string>()(m_shared_memory_name) + "_holders_lock");
        // {
        //     std::ofstream ofs(file_lock_mutex_holders_name, std::ofstream::trunc | std::ofstream::out);
        //     if (!ofs) {
        //         throw std::runtime_error("could not create file '" + file_lock_mutex_holders_name + "'");
        //     }

        // }
        // m_mutex_holders = boost::interprocess::file_lock(file_lock_mutex_holders_name.c_str());

        // std::cout << "shared_memory creating watcher process" << std::endl;
        // boost::process::spawn(
        //     boost::process::args = { "watcher", shared_memory_name }
        // );
    }

    shared_memory_t::shared_memory_t(const std::string& shared_memory_name) {
        m_owner = false;
        m_shared_memory_name = shared_memory_name;

        using namespace boost::interprocess;
        
        m_managed_shared_memory = managed_shared_memory(open_only, m_shared_memory_name.c_str());

        // m_holders = find_named<decltype(m_holders)::value_type>(std::hash<std::string>()(m_shared_memory_name) + "_holders");
        // boost::interprocess::scoped_lock guard_mutex_holders(m_mutex_holders);
        // m_holders->insert(boost::this_process::get_id());
    }

    shared_memory_t::~shared_memory_t() {
        using namespace boost::interprocess;

        // boost::interprocess::scoped_lock guard_mutex_holders(m_mutex_holders);
        // m_holders->erase(boost::this_process::get_id());

        if (m_owner) {
            shared_memory_object::remove(m_shared_memory_name.c_str());
        }

        // if (m_holders->empty()) {                
        //     shared_memory_object::remove(m_shared_memory_name.c_str());
        //     std::string file_lock_mutex_holders_name(std::hash<std::string>()(m_shared_memory_name) + "_holders_lock");
        //     std::remove(file_lock_mutex_holders_name.c_str());
        // }
    }

    void init(const std::string& shared_memory_name, size_t shared_memory_size) {
        g_shared_memory = new shared_memory_t(shared_memory_name, shared_memory_size);
    }

    void init(const std::string& shared_memory_name) {
        g_shared_memory = new shared_memory_t(shared_memory_name);
    }

    void deinit() {
        delete g_shared_memory;
    }
}
