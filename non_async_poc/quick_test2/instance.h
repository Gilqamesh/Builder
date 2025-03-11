#ifndef INSTANCE_H
# define INSTANCE_H

# include "type.h"

# include <chrono>
# include <optional>

using clock_type_t = chrono::high_resolution_clock;
using time_type_t  = chrono::time_point<clock_type_t>;

void set_time_init();

time_type_t get_time();
time_type_t get_time_init();

struct instance_t {
    type_t* m_type;
    unordered_map<type_t*, state_t> m_states;

    time_type_t m_t_start;
    time_type_t m_t_success;
    time_type_t m_t_failure;
    time_type_t m_t_finish;
    time_type_t m_t_propagate;

    unordered_map<string, unordered_map<instance_t*, size_t>> m_inputs;
    unordered_map<instance_t*, size_t> m_outputs;

    instance_t(type_t* type);
    ~instance_t();

    instance_t(const instance_t& other) = delete;
    instance_t& operator=(const instance_t& other) = delete;

    instance_t(instance_t&& other) noexcept = default;
    instance_t& operator=(instance_t&& other) noexcept = default;

    template <typename T>
    const T& read(const string& name) const;

    template <typename T>
    T& write(const string& name);

    optional<instance_t> convert(type_t* type) const;

    void add_input(const string& slot_name, instance_t* input, size_t n = 1);
    void remove_input(const string& slot_name, instance_t* input, size_t n = -1);

    void isolate();

    bool run(time_type_t t_propagate = get_time());
    bool run_force(time_type_t t_propagate = get_time());

private:
    template <typename T>
    const T* find(const string& name) const;

    template <typename T>
    T* find(const string& name);

    template <typename T>
    const T* find(const string& name, type_t* type, unordered_set<type_t*>& seen) const;

    template <typename T>
    T* find(const string& name, type_t* type, unordered_set<type_t*>& seen);

    bool instantiate(type_t* type, unordered_set<type_t*>& seen);
    bool instantiate();
    void destruct(type_t* type, unordered_set<type_t*>& seen);
    void destruct();

    bool convert(type_t* to, const instance_t& prev, instance_t& result) const;

    bool run_preamble(time_type_t t_propagate);
    bool run_impl();
    bool run_postamble(time_type_t t_propagate);
};

# include "instance_impl.h"

#endif // INSTANCE_H
