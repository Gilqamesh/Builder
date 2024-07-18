#include "mem.h"

#include <iostream>
#include <cstdio>

#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>

namespace ipc_mem {

template <typename T>
std::ostream& operator<<(std::ostream& os, const offset_ptr_t<T>& self) {
    if (!self) {
        os << self.get();
    } else {
        os << *self;
    }

    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const shared_ptr_t<T>& self) {
    if (!self) {
        os << self.get();
    } else {
        os << *self;
    }

    return os;
}

template <typename T>
shared_allocator_t<T>::shared_allocator_t():
    base(g_shared_memory->m_managed_shared_memory.get_segment_manager()) {
}

template <typename T, class derived_t>
multi_accessed_data_t<T, derived_t>::multi_accessed_data_t():
    multi_accessed_data_t(T{})
{
}

template <typename T, class derived_t>
multi_accessed_data_t<T, derived_t>::multi_accessed_data_t(T&& data):
    m_data(std::move(data)),
    m_readers(0),
    m_writers(0),
    m_owners_top(0)
{
    // for (size_t i = 0; i < max_number_of_concurrent_owners; ++i) {
    //     m_owners[i].m_id = owner_id_t{};
    //     m_owners[i].m_ownership_count = 0;
    //     m_owners[i].m_prev_operation = operation_t::NONE;
    // }
}

template <typename T, class derived_t>
void multi_accessed_data_t<T, derived_t>::read(const std::function<void(multi_accessed_data_t&, T&)>& fn) {
    // note: no need to sync this logic, as only this thread can change our own ownership count, and we are protected against writers in case we have ownership here
    if (0 < ownership_count()) {
        operation_t prev_operation = increment_ownership_count(operation_t::READ);
        fn(*this, m_data);
        decrement_ownership_count(prev_operation);
        return ;
    }

    {
        guard_mutex_t guard_mutex_writers(m_mutex_writers);
        m_cv_writers.wait(guard_mutex_writers, [this]() { return m_writers == 0; });
    }

    guard_shared_mutex_t guard_mutex_data(m_mutex_data);

    {
        guard_mutex_t guard_mutex_readers(m_mutex_readers);
        ++m_readers;
    }

    operation_t prev_operation = increment_ownership_count(operation_t::READ);
    fn(*this, m_data);
    decrement_ownership_count(prev_operation);

    {
        guard_mutex_t guard_mutex_readers(m_mutex_readers);
        --m_readers;
    }
}

template <typename T, class derived_t>
void multi_accessed_data_t<T, derived_t>::write(const std::function<void(multi_accessed_data_t&, T&)>& fn) {
    // note: no need to sync this logic, as only this thread can change our own ownership count, and we are protected against writers in case we have ownership here
    if (0 < ownership_count()) {
        operation_t prev_operation = increment_ownership_count(operation_t::WRITE);
        fn(*this, m_data);
        decrement_ownership_count(prev_operation);
        return ;
    }

    {
        guard_mutex_t guard_mutex_writers(m_mutex_writers);
        ++m_writers;
    }

    guard_mutex_shared_t guard_mutex_data(m_mutex_data);

    operation_t prev_operation = increment_ownership_count(operation_t::WRITE);
    fn(*this, m_data);
    decrement_ownership_count(prev_operation);

    {
        guard_mutex_t guard_mutex_writers(m_mutex_writers);
        if (--m_writers == 0) {
            m_cv_writers.notify_all();
        }
    }
}

template <typename T, class derived_t>
int multi_accessed_data_t<T, derived_t>::ownership_count() {
    guard_mutex_t guard_owners_mutex(m_owners_mutex);
    owner_id_t id = owner_id_namespace::get_id();
    for (const auto& owner : m_owners) {
        if (owner.m_id == id) {
            return owner.m_ownership_count;
        }
    }

    return 0;
}

template <typename T, class derived_t>
multi_accessed_data_t<T, derived_t>::operation_t
multi_accessed_data_t<T, derived_t>::increment_ownership_count(operation_t operation) {
    operation_t result = operation_t::NONE;

    guard_mutex_t guard_owners_mutex(m_owners_mutex);
    owner_id_t id = owner_id_namespace::get_id();
    auto owner_it = m_owners.begin();
    auto hole_it = m_owners.end();
    
    while (owner_it != m_owners.end()) {
        if (owner_it->m_id == id) {
            break ;
        }
        if (hole_it == m_owners.end() && owner_it->m_ownership_count == 0) {
            hole_it = owner_it;
        }

        ++owner_it;
    }

    if (owner_it == m_owners.end()) {
        if (hole_it != m_owners.end()) {
            owner_it = hole_it;
        } else {
            owner_it = m_owners.insert(owner_it, std::move(owner_t()));
        }

        assert(owner_it != m_owners.end());

        assert(owner_it->m_id == owner_id_t{});
        owner_it->m_id = id;
        assert(owner_it->m_ownership_count == 0);
        owner_it->m_ownership_count = 1;
        assert(owner_it->m_prev_operation == operation_t::NONE);
        result = owner_it->m_prev_operation;
        owner_it->m_prev_operation = operation;
    } else {
        assert(owner_it->m_id == id);
        ++owner_it->m_ownership_count;
        if (operation == operation_t::WRITE && owner_it->m_prev_operation == operation_t::READ) {
            throw std::runtime_error("A write operation cannot depend on a read operation");
        }
        result = owner_it->m_prev_operation;
        owner_it->m_prev_operation = operation;
    }

    return result;
}

template <typename T, class derived_t>
void multi_accessed_data_t<T, derived_t>::decrement_ownership_count(operation_t operation) {
    guard_mutex_t guard_owners_mutex(m_owners_mutex);
    owner_id_t id = owner_id_namespace::get_id();
    for (auto& owner : m_owners) {
        if (owner.m_id == id) {
            assert(0 < owner.m_ownership_count);
            owner.m_prev_operation = operation;
            if (--owner.m_ownership_count == 0) {
                owner.m_id = owner_id_t{};
                owner = m_owners.back();
                m_owners.back() = std::move(owner_t());
                m_owners.shrink_to_fit();
            }
            return ;
        }
    }

    assert(0 && "the thread did not previously have ownership");
}

template <typename shared_container_t>
container_base_t<shared_container_t>::container_base_t():
    base(boost::move(shared_container_t(shared_allocator_t<typename shared_container_t::value_type>()))) {
}

template <typename T>
shared_vector_base_t<T>::shared_vector_base_t():
    base(shared_allocator_t<T>())
{
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const shared_vector_t<T>& self) {
    const_cast<shared_vector_t<T>&>(self).read([&os](auto& vector) {
        os << "{ ";
        for (auto& element : vector) {
            os << element << " ";
        }
        os << "}";
    });

    return os;
}

template <typename K, typename V>
std::ostream& operator<<(std::ostream& os, const shared_map_t<K, V>& self) {
    const_cast<shared_map_t<K, V>&>(self).read([&os](auto& map) {
        os << "{ ";
        for (auto& pair : map) {
            os << "{ " << pair.first << " -> " << pair.second << " } ";
        }
        os << "}";
    });

    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const shared_set_t<T>& self) {
    const_cast<shared_set_t<T>&>(self).read([&os](auto& set) {
        os << "{ ";
        for (auto& element : set) {
            os << element << " ";
        }
        os << "}";
    });

    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const shared_deque_t<T>& self) {
    const_cast<shared_deque_t<T>&>(self).read([&os](auto& deque) {
        os << "{ ";
        for (auto& element : deque) {
            os << element << " ";
        }
        os << "}";
    });

    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const shared_string_t<T>& self) {
    const_cast<shared_string_t<T>&>(self).read([&os](auto& string) {
        os << "\"" << string << "\"";
    });

    return os;
}

template <typename T>
shared_string_t<T>::shared_string_t(const std::basic_string<T>& str):
    base(boost::move(value_type(str.c_str(), shared_allocator_t<T>()))) {
    this->write([&str](auto& s) {
        s.assign(str.begin(), str.end());
    });
}

template <typename T>
bool operator<(const shared_string_t<T>& l, const shared_string_t<T>& r) {
    return l.m_data < r.m_data;
}

template <typename T>
std::basic_string<T> operator+(const std::basic_string<T>& l, const shared_string_t<T>& r) {
    return l + std::basic_string<T>(r.begin(), r.end());
}

template <typename T>
std::basic_string<T> operator+(const shared_string_t<T>& l, const std::basic_string<T>& r) {
    return std::basic_string<T>(l.begin, l.end()) + r;
}

template <typename T>
offset_ptr_t<T> malloc_many_named(const std::string& object_name, size_t number_of_objects) {
    offset_ptr_t<T> result = g_shared_memory->m_managed_shared_memory.construct<T>(object_name.c_str())[number_of_objects]();
    if (!result) {
        throw std::runtime_error("out of memory");
    }
    std::cout << "shared_memory memory left: " << g_shared_memory->m_managed_shared_memory.get_free_memory() << std::endl;
    return result;
}

template <typename T, typename... Args>
offset_ptr_t<T> malloc_named(const std::string& object_name, Args&&... args) {
    offset_ptr_t<T> result = g_shared_memory->m_managed_shared_memory.construct<T>(object_name.c_str())(std::forward<Args>(args)...);
    if (!result) {
        throw std::runtime_error("out of memory");
    }
    std::cout << "shared_memory memory left: " << g_shared_memory->m_managed_shared_memory.get_free_memory() << std::endl;
    return result;
}

template <typename T>
offset_ptr_t<T> malloc_many(size_t number_of_objects) {
    offset_ptr_t<T> result = g_shared_memory->m_managed_shared_memory.construct<T>(boost::interprocess::anonymous_instance)[number_of_objects]();
    if (!result) {
        throw std::runtime_error("out of memory");
    }
    std::cout << "shared_memory memory left: " << g_shared_memory->m_managed_shared_memory.get_free_memory() << std::endl;
    return result;
}

template <typename T, typename... Args>
offset_ptr_t<T> malloc(Args&&... args) {
    offset_ptr_t<T> result = g_shared_memory->m_managed_shared_memory.construct<T>(boost::interprocess::anonymous_instance)(std::forward<Args>(args)...);
    if (!result) {
        throw std::runtime_error("out of memory");
    }
    std::cout << "shared_memory memory left: " << g_shared_memory->m_managed_shared_memory.get_free_memory() << std::endl;
    return result;
}

template <typename T, typename... Args>
shared_ptr_t<T> malloc_shared(Args&&... args) {
    shared_ptr_t<T> result = 
    boost::interprocess::make_managed_shared_ptr(
        g_shared_memory->m_managed_shared_memory.construct<T>(boost::interprocess::anonymous_instance)(std::forward<Args>(args)...),
        g_shared_memory->m_managed_shared_memory
    );

    if (!result) {
        throw std::runtime_error("out of memory");
    }
    std::cout << "shared_memory memory left: " << g_shared_memory->m_managed_shared_memory.get_free_memory() << std::endl;
    return result;
}

template <typename T>
void free(abs_ptr_t<T> ptr) {
    g_shared_memory->m_managed_shared_memory.destroy_ptr(ptr);
    std::cout << "shared_memory memory left: " << g_shared_memory->m_managed_shared_memory.get_free_memory() << std::endl;
}

template <typename T>
void free(offset_ptr_t<T> ptr) {
    g_shared_memory->m_managed_shared_memory.destroy_ptr(ptr.get());
    std::cout << "shared_memory memory left: " << g_shared_memory->m_managed_shared_memory.get_free_memory() << std::endl;
}

template <typename T>
offset_ptr_t<T> find_named(const std::string& object_name) {
    offset_ptr_t<T> result = g_shared_memory->m_managed_shared_memory.find<T>(object_name.c_str()).first;
    if (!result) {
        throw std::runtime_error("did not find named object '" + object_name + "'");
    }
    return result;
}

} // namespace ipc_mem
