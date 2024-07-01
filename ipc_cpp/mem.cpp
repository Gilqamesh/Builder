#include "mem.h"

#include <stdexcept>

shared_memory_t::shared_memory_t(const std::string& shared_memory_name, size_t shared_memory_size) {
    m_is_owner = 1;
    m_shared_memory_name = shared_memory_name;

    using namespace boost::interprocess;

    shared_memory_object::remove(m_shared_memory_name.c_str());
    m_managed_shared_memory = managed_shared_memory(create_only, m_shared_memory_name.c_str(), shared_memory_size);

    if (m_managed_shared_memory.get_size() < shared_memory_size) {
        throw std::runtime_error("got less memory than expected");
    }
}

shared_memory_t::shared_memory_t(const std::string& shared_memory_name) {
    m_is_owner = 0;
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
