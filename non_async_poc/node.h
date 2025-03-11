#ifndef NODE_H
# define NODE_H

# include <chrono>
# include <vector>
# include <functional>
# include <string>
# include <thread>

using namespace std;

using clock_type_t = chrono::high_resolution_clock;
using time_type_t  = chrono::time_point<clock_type_t>;

void set_time_init();

time_type_t get_time();
time_type_t get_time_init();

struct signature_base_t {
    unordered_map<string, shared_ptr<void>> m_symbol_str_to_fn_pointer;
};

template <typename signature_t>
struct signature_derived_t : public signature_base_t {
    signature_t add(const string& symbol_name, const signature_t& fn);
    signature_t find(const string& symbol_name);
};

struct interface_t {
    unordered_map<const char*, signature_base_t*> m_signature_str_to_signature_base;

    template <typename signature_t>
    signature_t add(const string& symbol_name, const signature_t& fn);

    template <typename signature_t>
    signature_t find(const string& symbol_name);

    void print(int depth = 0);
};

struct state_base_t {
    struct entry_t {
        void* data;
        function<void(void*)> dtor;
    };
    unordered_map<string, entry_t> m_entries;

    ~state_base_t();

    template <typename T, typename... Args>
    T* add(const string& field_name, Args&&... args);

    template <typename T>
    T* find(const string& field_name);
};

struct node_t {
    time_type_t m_t_start;
    time_type_t m_t_success;
    time_type_t m_t_failure;
    time_type_t m_t_finish;
    time_type_t m_t_propagate;

    unordered_map<const char*, unordered_map<node_t*, size_t>> m_inputs;
    unordered_map<node_t*, size_t> m_outputs;
    
    interface_t m_interface;
    
    state_base_t m_state;

    bool run_preamble(time_type_t t_propagate);
    bool run_impl();
    bool run_postamble(time_type_t t_propagate);

    template <typename slot_t>
    void add_slot();

    template <typename slot_t>
    unordered_map<node_t*, size_t>& find_slot();
public:
    node_t(
        const function<bool(node_t&)>& constructor_fn,
        const function<bool(node_t&)>& run_fn,
        const function<string(const node_t&)>& describe_fn,
        const function<void(node_t&)>& destructor_fn
    );
    ~node_t();

    template <typename slot_t>
    void add_input(node_t* input, size_t n = 1);

    template <typename slot_t>
    void remove_input(node_t* input, size_t n = -1);

    void isolate();

    bool run(time_type_t t_propagate);
    bool run_force(time_type_t t_propagate);

    template <typename signature_t>
    signature_t find_interface(const string& symbol_name);

    string describe();

    template <typename T>
    T* add_state();

    template <typename T>
    void remove_state();

    template <typename T>
    const T* read_state() const;

    template <typename T>
    T* write_state();

    const decltype(m_inputs)& read_inputs() const;
    decltype(m_inputs)& write_inputs();
};

// note: might be better to just let applications make their own functions like this
template <typename node_type_t>
node_type_t* find_or_create();

# include "node_impl.h"

#endif // NODE_H
