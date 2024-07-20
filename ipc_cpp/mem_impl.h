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

template <typename T>
thread_safe_data_t<T>::thread_safe_data_t():
    m_data{},
    m_readers(0),
    m_writers(0)
{
}

template <typename T>
thread_safe_data_t<T>::thread_safe_data_t(T&& data):
    m_data(boost::move(data)),
    m_readers(0),
    m_writers(0)
{
}

template <typename T>
void thread_safe_data_t<T>::read(const std::function<void(T&)>& fn) {
    {
        std::unique_lock<std::mutex> guard_mutex_writers(m_mutex_writers);
        m_cv_writers.wait(guard_mutex_writers, [this]() { return m_writers == 0; });
    }

    std::shared_lock<std::shared_mutex> guard_mutex_data(m_mutex_data);

    {
        std::unique_lock<std::mutex> guard_mutex_readers(m_mutex_readers);
        ++m_readers;
        // std::cout << std::this_thread::get_id() << " readers: " << m_readers << std::endl;
    }

    fn(m_data);

    {
        std::unique_lock<std::mutex> guard_mutex_readers(m_mutex_readers);
        --m_readers;
    }
}

template <typename T>
void thread_safe_data_t<T>::write(const std::function<void(T&)>& fn) {
    {
        std::unique_lock<std::mutex> guard_mutex_writers(m_mutex_writers);
        ++m_writers;
        // std::cout << std::this_thread::get_id() << " writers: " << m_writers << std::endl;
    }

    std::unique_lock<std::shared_mutex> guard_mutex_data(m_mutex_data);

    fn(m_data);

    {
        std::unique_lock<std::mutex> guard_mutex_writers(m_mutex_writers);
        if (--m_writers == 0) {
            m_cv_writers.notify_all();
        }
    }
}

template <typename T>
process_safe_data_t<T>::process_safe_data_t():
    m_data{},
    m_readers(0),
    m_writers(0)
{
}

template <typename T>
process_safe_data_t<T>::process_safe_data_t(T&& data):
    m_data(boost::move(data)),
    m_readers(0),
    m_writers(0)
{
}

template <typename T>
void process_safe_data_t<T>::read(const std::function<void(T&)>& fn) {
    {
        boost::interprocess::scoped_lock guard_mutex_writers(m_mutex_writers);
        m_cv_writers.wait(guard_mutex_writers, [this]() { return m_writers == 0; });
    }


    boost::interprocess::sharable_lock guard_mutex_data(m_mutex_data);

    {
        boost::interprocess::scoped_lock guard_mutex_readers(m_mutex_readers);
        ++m_readers;
        // std::cout << std::this_thread::get_id() << " readers: " << m_readers << std::endl;
    }

    fn(m_data);

    {
        boost::interprocess::scoped_lock guard_mutex_readers(m_mutex_readers);
        --m_readers;
    }
}

template <typename T>
void process_safe_data_t<T>::write(const std::function<void(T&)>& fn) {
    {
        boost::interprocess::scoped_lock guard_mutex_writers(m_mutex_writers);
        ++m_writers;
        // std::cout << std::this_thread::get_id() << " writers: " << m_writers << std::endl;
    }

    boost::interprocess::scoped_lock guard_mutex_data(m_mutex_data);

    fn(m_data);

    {
        boost::interprocess::scoped_lock guard_mutex_writers(m_mutex_writers);
        if (--m_writers == 0) {
            m_cv_writers.notify_all();
        }
    }
}

template <typename shared_container_t>
container_base_t<shared_container_t>::container_base_t():
    base(boost::move(shared_container_t(shared_allocator_t<typename shared_container_t::value_type>()))) {
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