#ifndef MEM_H
# define MEM_H

# include <string>
# include <iostream>
# include <boost/interprocess/managed_shared_memory.hpp>
# include <boost/interprocess/containers/vector.hpp>
# include <boost/interprocess/containers/set.hpp>
# include <boost/interprocess/containers/string.hpp>
# include <boost/interprocess/smart_ptr/shared_ptr.hpp>

template <typename T>
using shared_allocator_t = boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager>;

template <typename T>
using shared_vector_t = boost::interprocess::vector<T, shared_allocator_t<T>>;

template <typename T>
using shared_set_t = boost::interprocess::set<T, std::less<T>, shared_allocator_t<T>>;

template <typename T>
using shared_string_t = boost::interprocess::basic_string<T, std::char_traits<T>, shared_allocator_t<T>>;

template <typename T>
using shared_ptr_t = boost::interprocess::managed_shared_ptr<T, boost::interprocess::managed_shared_memory>::type;

template <typename T>
using offset_ptr_t = boost::interprocess::offset_ptr<T>;

template <typename T>
using abs_ptr_t = T*;

struct shared_memory_t {
    // creates shared memory
    shared_memory_t(const std::string& shared_memory_name, size_t shared_memory_size);

    // opens shared memory
    shared_memory_t(const std::string& shared_memory_name);

   // destroys shared memory if this was the creator
    ~shared_memory_t();

    // create a named array with default constructed elements
    template <typename T>
    offset_ptr_t<T> malloc_many_named(const std::string& object_name, size_t number_of_objects);

    // create a named object, forward arguments to its constructor
    template <typename T, typename... Args>
    offset_ptr_t<T> malloc_named(const std::string& object_name, Args... args);

    // create an anonymous array with default constructed elements
    template <typename T>
    offset_ptr_t<T> malloc_many(size_t number_of_objects);

    // create an anonymous object, forward arguments to its constructor
    template <typename T, typename... Args>
    offset_ptr_t<T> malloc(Args... args);

    // create an anonymous object wrapped in a shared ptr, forward arguments to its constructor
    template <typename T, typename... Args>
    shared_ptr_t<T> malloc_shared(Args... args);

    template <typename T>
    void free(offset_ptr_t<T> ptr);

    template <typename T>
    offset_ptr_t<T> find_named(const std::string& object_name);

    template <typename T>
    shared_allocator_t<T> get_allocator();

    std::string m_shared_memory_name;
    boost::interprocess::managed_shared_memory m_managed_shared_memory;
    int m_is_owner;
};

# include "mem_impl.h"

#endif // MEM_H
