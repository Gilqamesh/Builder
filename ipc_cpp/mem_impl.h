#include "mem.h"

template <typename T>
offset_ptr_t<T> shared_memory_t::malloc_many_named(const std::string& object_name, size_t number_of_objects) {
    offset_ptr_t<T> result = m_managed_shared_memory.construct<T>(object_name.c_str())[number_of_objects]();
    if (!result) {
        throw std::runtime_error("out of memory");
    }
    return result;
}

template <typename T, typename... Args>
offset_ptr_t<T> shared_memory_t::malloc_named(const std::string& object_name, Args... args) {
    offset_ptr_t<T> result = m_managed_shared_memory.construct<T>(object_name.c_str())(std::forward<Args>(args)...);
    if (!result) {
        throw std::runtime_error("out of memory");
    }
    return result;
}

template <typename T>
offset_ptr_t<T> shared_memory_t::malloc_many(size_t number_of_objects) {
    offset_ptr_t<T> result = m_managed_shared_memory.construct<T>(boost::interprocess::anonymous_instance)[number_of_objects]();
    if (!result) {
        throw std::runtime_error("out of memory");
    }
    return result;
}

template <typename T, typename... Args>
offset_ptr_t<T> shared_memory_t::malloc(Args... args) {
    offset_ptr_t<T> result = m_managed_shared_memory.construct<T>(boost::interprocess::anonymous_instance)(std::forward<Args>(args)...);
    if (!result) {
        throw std::runtime_error("out of memory");
    }
    return result;
}

template <typename T, typename... Args>
shared_ptr_t<T> shared_memory_t::malloc_shared(Args... args) {
    shared_ptr_t<T> result = 
    boost::interprocess::make_managed_shared_ptr(
        m_managed_shared_memory.construct<T>(boost::interprocess::anonymous_instance)(std::forward<Args>(args)...),
        m_managed_shared_memory
    );

    if (!result) {
        throw std::runtime_error("out of memory");
    }
    return result;
}

template <typename T>
offset_ptr_t<T> shared_memory_t::find_named(const std::string& object_name) {
    offset_ptr_t<T> result = m_managed_shared_memory.find<T>(object_name.c_str()).first;
    if (!result) {
        throw std::runtime_error("did not fiond named object");
    }
    return result;
}

template <typename T>
void shared_memory_t::free(offset_ptr_t<T> ptr) {
    m_managed_shared_memory.destroy_ptr(ptr.get());
}

template <typename T>
shared_allocator_t<T> shared_memory_t::get_allocator() {
    return shared_allocator_t<T>(m_managed_shared_memory.get_segment_manager());
}
