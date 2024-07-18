#ifndef IPC_MEM_H
# define IPC_MEM_H

# include <string>
# include <iostream>
# include <functional>
# include <mutex>
# include <shared_mutex>
# include <condition_variable>
# include <functional>

# include <boost/interprocess/managed_shared_memory.hpp>
# include <boost/interprocess/containers/vector.hpp>
# include <boost/interprocess/containers/map.hpp>
# include <boost/interprocess/containers/set.hpp>
# include <boost/interprocess/containers/string.hpp>
# include <boost/interprocess/containers/deque.hpp>
# include <boost/interprocess/smart_ptr/shared_ptr.hpp>
# include <boost/interprocess/sync/interprocess_condition.hpp>
# include <boost/interprocess/sync/interprocess_mutex.hpp>
# include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
# include <boost/interprocess/sync/interprocess_recursive_mutex.hpp>
# include <boost/process.hpp>

namespace ipc_mem {

struct shared_memory_t;
extern shared_memory_t* g_shared_memory;

template <typename T>
struct offset_ptr_t : public boost::interprocess::offset_ptr<T> {
    using value_type = T;
    using base = boost::interprocess::offset_ptr<T>;
    using base::base;
    using base::operator=;

    template <typename U>
    friend std::ostream& operator<<(std::ostream& os, const offset_ptr_t<U>& self);
};

template <typename T>
using shared_ptr_t = typename boost::interprocess::managed_shared_ptr<T, boost::interprocess::managed_shared_memory>::type;

template <typename T>
std::ostream& operator<<(std::ostream& os, const shared_ptr_t<T>& self);

template <typename T>
using abs_ptr_t = T*;

template <typename T>
struct shared_allocator_t : public boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager> {
    using base = boost::interprocess::allocator<T, boost::interprocess::managed_shared_memory::segment_manager>;

    shared_allocator_t();
};

// creates shared memory
void init(const std::string& shared_memory_name, size_t shared_memory_size);

// opens shared memory
void init(const std::string& shared_memory_name);

// destroys shared memory if this was the creator
void deinit();

// create a named array with default constructed elements
template <typename T>
offset_ptr_t<T> malloc_many_named(const std::string& object_name, size_t number_of_objects);

// create a named object, forward arguments to its constructor
template <typename T, typename... Args>
offset_ptr_t<T> malloc_named(const std::string& object_name, Args&&... args);

// create an anonymous array with default constructed elements
template <typename T>
offset_ptr_t<T> malloc_many(size_t number_of_objects);

// create an anonymous object, forward arguments to its constructor
template <typename T, typename... Args>
offset_ptr_t<T> malloc(Args&&... args);

// create an anonymous object wrapped in a shared ptr, forward arguments to its constructor
template <typename T, typename... Args>
shared_ptr_t<T> malloc_shared(Args&&... args);

template <typename T>
void free(abs_ptr_t<T> ptr);

template <typename T>
void free(offset_ptr_t<T> ptr);

template <typename T>
offset_ptr_t<T> find_named(const std::string& object_name);


template <typename condition_variable_t, typename guard_mutex_t>
concept concept_condition_variable_t = requires (
    condition_variable_t& condition_variable,
    guard_mutex_t& guard_mutex,
    const std::function<bool()>& predicate_fn
) {
    { condition_variable.wait(guard_mutex, predicate_fn) } -> std::same_as<void>;
    { condition_variable.notify_one() } -> std::same_as<void>;
    { condition_variable.notify_all() } -> std::same_as<void>;
};

template <typename owner_id_namespace, typename owner_id_t>
concept concept_owner_id_namespace = requires() {
    { owner_id_namespace::get_id() } -> std::same_as<owner_id_t>;
};

template <typename mutex_t>
concept concept_mutex_t = requires (mutex_t& mutex) {
    { mutex.lock() } -> std::same_as<void>;
    { mutex.unlock() } -> std::same_as<void>;
};

template <typename guard_mutex_t, typename mutex_t>
concept concept_guard_mutex_t = requires (guard_mutex_t& guard_mutex, mutex_t& mutex) {
    std::constructible_from<guard_mutex_t, mutex_t&>;
    { guard_mutex.lock() } -> std::same_as<void>;
    { guard_mutex.unlock() } -> std::same_as<void>;
};

template <typename T, class derived_t>
struct multi_accessed_data_t;

template <typename T>
struct multi_accessed_data_traits_t;

template <typename T>
struct thread_safe_data_t : public multi_accessed_data_t<T, thread_safe_data_t<T>> {
    template <typename U>
    using vector_t             = typename multi_accessed_data_traits_t<thread_safe_data_t>::template vector_t<U>;
    using mutex_t              = typename multi_accessed_data_traits_t<thread_safe_data_t>::mutex_t;
    using shared_mutex_t       = typename multi_accessed_data_traits_t<thread_safe_data_t>::shared_mutex_t;
    using condition_variable_t = typename multi_accessed_data_traits_t<thread_safe_data_t>::condition_variable_t;
    using owner_id_t           = typename multi_accessed_data_traits_t<thread_safe_data_t>::owner_id_t;
    using owner_id_namespace   = typename multi_accessed_data_traits_t<thread_safe_data_t>::owner_id_namespace;
    using guard_mutex_t        = typename multi_accessed_data_traits_t<thread_safe_data_t>::guard_mutex_t;
    using guard_mutex_shared_t = typename multi_accessed_data_traits_t<thread_safe_data_t>::guard_mutex_shared_t;
    using guard_shared_mutex_t = typename multi_accessed_data_traits_t<thread_safe_data_t>::guard_shared_mutex_t;
};

template <typename T>
struct multi_accessed_data_traits_t<thread_safe_data_t<T>> {
    template <typename U>
    using vector_t             = std::vector<U>;
    using mutex_t              = typename std::mutex;
    using shared_mutex_t       = typename std::shared_mutex;
    using condition_variable_t = typename std::condition_variable;
    using owner_id_t           = typename std::thread::id;
    struct owner_id_namespace {
        static owner_id_t get_id() {
            return std::this_thread::get_id();
        }
    };
    using guard_mutex_t        = typename std::unique_lock<mutex_t>;
    using guard_mutex_shared_t = typename std::unique_lock<shared_mutex_t>;
    using guard_shared_mutex_t = typename std::shared_lock<shared_mutex_t>;
};

template <typename T>
struct process_safe_data_t : public multi_accessed_data_t<T, process_safe_data_t<T>> {
    template <typename U>
    using vector_t             = typename multi_accessed_data_traits_t<process_safe_data_t>::template vector_t<U>;
    using mutex_t              = typename multi_accessed_data_traits_t<process_safe_data_t>::mutex_t;
    using shared_mutex_t       = typename multi_accessed_data_traits_t<process_safe_data_t>::shared_mutex_t;
    using condition_variable_t = typename multi_accessed_data_traits_t<process_safe_data_t>::condition_variable_t;
    using owner_id_t           = typename multi_accessed_data_traits_t<process_safe_data_t>::owner_id_t;
    using owner_id_namespace   = typename multi_accessed_data_traits_t<process_safe_data_t>::owner_id_namespace;
    using guard_mutex_t        = typename multi_accessed_data_traits_t<process_safe_data_t>::guard_mutex_t;
    using guard_mutex_shared_t = typename multi_accessed_data_traits_t<process_safe_data_t>::guard_mutex_shared_t;
    using guard_shared_mutex_t = typename multi_accessed_data_traits_t<process_safe_data_t>::guard_shared_mutex_t;
};

template <typename T>
struct shared_vector_base_t;

template <typename T>
struct multi_accessed_data_traits_t<process_safe_data_t<T>> {
    template <typename U>
    using vector_t             = shared_vector_base_t<U>;
    using mutex_t              = typename boost::interprocess::interprocess_mutex;
    using shared_mutex_t       = typename boost::interprocess::interprocess_sharable_mutex;
    using condition_variable_t = typename boost::interprocess::interprocess_condition;
    using owner_id_t           = decltype(boost::this_process::get_id());
    struct owner_id_namespace {
        static owner_id_t get_id() {
            return boost::this_process::get_id();
        }
    };
    using guard_mutex_t        = typename boost::interprocess::scoped_lock<mutex_t>;
    using guard_mutex_shared_t = typename boost::interprocess::scoped_lock<shared_mutex_t>;
    using guard_shared_mutex_t = typename boost::interprocess::sharable_lock<shared_mutex_t>;
};

// todo: implement robustness
template <typename T, class derived_t>
struct multi_accessed_data_t {
    template <typename U>
    using vector_t             = typename multi_accessed_data_traits_t<derived_t>::template vector_t<U>;
    using mutex_t              = typename multi_accessed_data_traits_t<derived_t>::mutex_t;
    using shared_mutex_t       = typename multi_accessed_data_traits_t<derived_t>::shared_mutex_t;
    using condition_variable_t = typename multi_accessed_data_traits_t<derived_t>::condition_variable_t;
    using owner_id_t           = typename multi_accessed_data_traits_t<derived_t>::owner_id_t;
    using owner_id_namespace   = typename multi_accessed_data_traits_t<derived_t>::owner_id_namespace;
    using guard_mutex_t        = typename multi_accessed_data_traits_t<derived_t>::guard_mutex_t;
    using guard_mutex_shared_t = typename multi_accessed_data_traits_t<derived_t>::guard_mutex_shared_t;
    using guard_shared_mutex_t = typename multi_accessed_data_traits_t<derived_t>::guard_shared_mutex_t;

    static_assert(concept_mutex_t<mutex_t>);
    static_assert(concept_mutex_t<shared_mutex_t>);
    static_assert(concept_condition_variable_t<condition_variable_t, guard_mutex_t>);
    static_assert(concept_owner_id_namespace<owner_id_namespace, owner_id_t>);
    static_assert(concept_guard_mutex_t<guard_mutex_t, mutex_t>);
    static_assert(concept_guard_mutex_t<guard_shared_mutex_t, shared_mutex_t>);

    multi_accessed_data_t();

    multi_accessed_data_t(T&& data);

    // use when there are 0 write operations on data
    void read(const std::function<void(multi_accessed_data_t&, T&)>& fn);

    // use when there is at least 1 write operation on data
    void write(const std::function<void(multi_accessed_data_t&, T&)>& fn);

    T m_data;
    shared_mutex_t m_mutex_data;

    size_t m_readers;
    mutex_t m_mutex_readers;

    size_t m_writers;
    mutex_t m_mutex_writers;
    condition_variable_t m_cv_writers;

    enum class operation_t : int {
        NONE,
        WRITE,
        READ
    };
    struct owner_t {
        owner_t() {
            m_id = owner_id_t{};
            m_ownership_count = 0;
            m_prev_operation = operation_t::NONE;
        }

        owner_id_t m_id;
        int m_ownership_count;
        operation_t m_prev_operation;
    };
    vector_t<owner_t> m_owners;
    // ptr_t<owner_t> m_owners;
    size_t m_owners_top;
    mutex_t m_owners_mutex;
    int  ownership_count();
    operation_t increment_ownership_count(operation_t operation); // returns previous operation
    void decrement_ownership_count(operation_t operation);
};

template <typename shared_container_t>
struct container_base_t : public process_safe_data_t<shared_container_t> {
    using base = process_safe_data_t<shared_container_t>;
    using base::base;
    using base::operator=;

    container_base_t();
};

template <typename T>
struct shared_vector_base_t : public boost::interprocess::vector<T, shared_allocator_t<T>> {
    using base = boost::interprocess::vector<T, shared_allocator_t<T>>;
    using base::base;
    using base::operator=;

    shared_vector_base_t();
};

template <typename T>
struct shared_vector_t : public container_base_t<shared_vector_base_t<T>> {
    using base = container_base_t<shared_vector_base_t<T>>;
    using base::base;
    using base::operator=;

    template <typename U>
    friend std::ostream& operator<<(std::ostream& os, const shared_vector_t<U>& self);
};

template <typename K, typename V>
struct shared_map_t : public container_base_t<boost::interprocess::map<K, V, std::less<K>, shared_allocator_t<std::pair<const K, V>>>> {
    using value_type = boost::interprocess::map<K, V, std::less<K>, shared_allocator_t<std::pair<const K, V>>>;
    using base = container_base_t<value_type>;
    using base::base;
    using base::operator=;

    template <typename K2, typename V2>
    friend std::ostream& operator<<(std::ostream& os, const shared_map_t<K2, V2>& self);
};

template <typename T>
struct shared_set_t : public container_base_t<boost::interprocess::set<T, std::less<T>, shared_allocator_t<T>>> {
    using value_type = boost::interprocess::set<T, std::less<T>, shared_allocator_t<T>>;
    using base = container_base_t<value_type>;
    using base::base;
    using base::operator=;

    template <typename U>
    friend std::ostream& operator<<(std::ostream& os, const shared_set_t<U>& self);
};

template <typename T>
struct shared_deque_t : public container_base_t<boost::interprocess::deque<T, shared_allocator_t<T>>> {
    using value_type = boost::interprocess::deque<T, shared_allocator_t<T>>;
    using base = container_base_t<value_type>;
    using base::base;
    using base::operator=;

    template <typename U>
    friend std::ostream& operator<<(std::ostream& os, const shared_deque_t<U>& self);
};

template <typename T>
struct shared_string_t : public container_base_t<boost::interprocess::basic_string<T, std::char_traits<T>, shared_allocator_t<T>>> {
    using value_type = boost::interprocess::basic_string<T, std::char_traits<T>, shared_allocator_t<T>>;
    using base = container_base_t<value_type>;
    using base::base;
    using base::operator=;

    shared_string_t(const std::basic_string<T>& str);

    template <typename U>
    friend std::ostream& operator<<(std::ostream& os, const shared_string_t<U>& self);
};

template <typename T>
bool operator<(const shared_string_t<T>& l, const shared_string_t<T>& r);

template <typename T>
std::basic_string<T> operator+(const std::basic_string<T>& l, const shared_string_t<T>& r);

template <typename T>
std::basic_string<T> operator+(const shared_string_t<T>& l, const std::basic_string<T>& r);

struct shared_memory_t {
    // creates shared memory
    shared_memory_t(const std::string& shared_memory_name, size_t shared_memory_size);

    // opens shared memory
    shared_memory_t(const std::string& shared_memory_name);

    // destroys shared memory if this was the creator
    ~shared_memory_t();

    std::string m_shared_memory_name;
    boost::interprocess::managed_shared_memory m_managed_shared_memory;

    bool m_is_owner;
};

} // namespace ipc_mem

# include "mem_impl.h"

#endif // IPC_MEM_H
