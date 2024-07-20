#include "mem.h"

#include <stdexcept>

namespace ipc_mem {
    shared_memory_t* g_shared_memory = 0;

    shared_memory_t::shared_memory_t(const std::string& shared_memory_name, size_t shared_memory_size) {
        m_is_owner = true;
        m_shared_memory_name = shared_memory_name;

        using namespace boost::interprocess;

        shared_memory_object::remove(m_shared_memory_name.c_str());
        m_managed_shared_memory = managed_shared_memory(create_only, m_shared_memory_name.c_str(), shared_memory_size);

        if (m_managed_shared_memory.get_size() < shared_memory_size) {
            throw std::runtime_error("got less memory than expected");
        }
        std::cout << "shared_memory memory left: " << m_managed_shared_memory.get_size() << std::endl;

        std::cout << "shared_memory creating watcher process" << std::endl;
        boost::process::spawn(
            boost::process::args = { "watcher", shared_memory_name }
        );
    }

    shared_memory_t::shared_memory_t(const std::string& shared_memory_name) {
        m_is_owner = false;
        m_shared_memory_name = shared_memory_name;

        using namespace boost::interprocess;
        
        m_managed_shared_memory = managed_shared_memory(open_only, m_shared_memory_name.c_str());
    }

    shared_memory_t::~shared_memory_t() {
        using namespace boost::interprocess;

        if (m_is_owner) {
            shared_memory_object::remove(m_shared_memory_name.c_str());
        }
    }

    bool exists() {
        using namespace boost::interprocess;

        try {
            managed_shared_memory(open_only, g_shared_memory->m_shared_memory_name.c_str());
            
            return true;
        } catch (...) {
            return false;
        }
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
