#ifndef NODE_H
# define NODE_H

# include <chrono>
# include <vector>
# include <functional>
# include <string>

using namespace std;

using clock_type_t = chrono::high_resolution_clock;
using time_type_t  = chrono::time_point<clock_type_t>;

void set_time_init();

time_type_t get_time();
time_type_t get_time_init();

struct memory_slice_t {
    void* memory;
    size_t size;
};

struct node_t {
protected:
    time_type_t m_t_start;
    time_type_t m_t_success;
    time_type_t m_t_failure;
    time_type_t m_t_finish;
    time_type_t m_t_propagate;

    vector<node_t*> m_inputs;
    vector<node_t*> m_outputs;

    memory_slice_t m_result;

    function<bool(const vector<node_t*>&, memory_slice_t&)> m_run_fn;
    function<void(const memory_slice_t&, string&)> m_describe_fn;
    function<void(node_t&)> m_deinit_fn;

    bool run_preamble(time_type_t t_propagate);
    bool run_impl();
    bool run_postamble(time_type_t t_propagate);
public:
    node_t(
        const function<bool(node_t&)>& init_fn,
        function<bool(const vector<node_t*>&, memory_slice_t&)>&& run_fn,
        function<void(const memory_slice_t&, string&)>&& describe_fn,
        function<void(node_t&)>&& deinit_fn
    );
    ~node_t();

    void add_input(node_t* input, size_t position = -1);
    void remove_all_input(node_t* input); // remove all occurance stable remove
    void remove_input(node_t* input); // remove first occurance, stable
    void isolate();

    void describe(string& str);

    bool run(time_type_t t_propagate);
    bool run_force(time_type_t t_propagate);
    memory_slice_t& write();
};

#endif // NODE_H
