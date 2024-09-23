#include "node.h"

# include <algorithm>
# include <iostream>
# include <cassert>

#define LINE() cout << __LINE__ << endl

static time_type_t t_init;

void set_time_init() {
    t_init = get_time();
}

time_type_t get_time() {
    return std::chrono::high_resolution_clock::now();
}

time_type_t get_time_init() {
    return t_init;
}

template <typename T>
bool erase_stable(vector<T>& v, const T& val, size_t p = 0) {
    for (size_t i = p; i < v.size(); ++i) {
        if (v[i] == val) {
            for (size_t j = i; j < v.size() - 1; ++j) {
                v[i] = move(v[i + 1]);
            }
            v.pop_back();
            return true;
        }
    }
    return false;
}

// removes all occurance of 'val' from 'v', same ordering of elements is guaranteed
// returns number of removed elements
template <typename T>
size_t erase_all_stable(vector<T>& v, const T& val) {
    size_t result = 0;
    size_t p = 0;
    while (p < v.size()) {
        if (erase_stable(v, val, p)) {
            ++result;
        } else {
            ++p;
        }
    }
    return result;
}

template <typename T>
bool erase_unstable(vector<T>& v, const T& val, size_t p = 0) {
    for (size_t i = p; i < v.size(); ++i) {
        if (v[i] == val) {
            if (i + 1 < v.size()) {
                v[i] = move(v.back());
            }
            v.pop_back();
            return true;
        }
    }
    return false;
}

// removes all occurance of 'val' from 'v', same ordering of elements are not guaranteed
// returns number of removed elements
template <typename T>
size_t erase_all_unstable(vector<T>& v, const T& val) {
    size_t result = 0;
    size_t p = 0;
    while (p < v.size()) {
        if (erase_unstable(v, val, p)) {
            ++result;
        } else {
            ++p;
        }
    }
    return result;
}

node_t::node_t(
    const function<void(node_t&)>& init_fn,
    function<bool(const vector<node_t*>&, memory_slice_t&)>&& run_fn,
    function<void(const memory_slice_t&, string&)>&& describe_fn,
    function<void(node_t&)>&& deinit_fn
):
    m_run_fn(move(run_fn)),
    m_describe_fn(move(describe_fn)),
    m_deinit_fn(move(deinit_fn))
{
    m_result.memory = 0;
    m_result.size = 0;

    init_fn(*this);
}

node_t::~node_t() {
    isolate();
    m_deinit_fn(*this);
}

bool node_t::run_preamble(time_type_t t_propagate) {
    if (t_propagate < m_t_propagate) {
        return false;
    }

    function<void(node_t* node)> propagate_time;
    propagate_time = [&propagate_time, t_propagate](node_t* node) {
        if (t_propagate <= node->m_t_propagate) {
            return ;
        }
        node->m_t_propagate = t_propagate;

        for (node_t* output : node->m_outputs) {
            propagate_time(output);
        }
    };
    propagate_time(this);

    if (m_inputs.empty()) {
        return true;
    }

    time_type_t most_recent_input;
    for (node_t* input : m_inputs) {
        if (input->m_t_propagate == t_propagate) {
            if (input->m_t_start < t_propagate) {
                return false;
            }
        }
        if (input->m_t_success < input->m_t_failure) {
            return false;
        }
        most_recent_input = max(most_recent_input, input->m_t_success);
    }

    if (most_recent_input <= m_t_success) {
        return false;
    }

    if (m_outputs.empty()) {
        return true;
    }

    time_type_t most_oldest_output = get_time();
    for (node_t* output : m_outputs) {
        most_oldest_output = min(most_oldest_output, output->m_t_success);
    }

    if (most_recent_input <= most_oldest_output) {
        return false;
    }

    return true;
}

bool node_t::run_impl() {
    m_t_start = get_time();
    if (!m_run_fn(m_inputs, m_result)) {
        m_t_failure = get_time();
        m_t_finish = m_t_failure;
        return false;
    }
    m_t_success = get_time();
    m_t_finish = m_t_success;

    return true;
}

bool node_t::run_postamble(time_type_t t_propagate) {
    // todo: use queue to incorporate looping constructs, otherwise stack blows up

    for (node_t* output : m_outputs) {
        if (output->m_t_start < m_t_success) {
            (void) output->run(t_propagate);
        }
    }

    return true;
}

void node_t::add_input(node_t* input, size_t position) {
    input->m_outputs.push_back(this);

    if (m_inputs.size() <= position) {
        position = m_inputs.size();
    }

    m_inputs.push_back(0);
    for (size_t i = m_inputs.size() - 1; position < i; --i) {
        m_inputs[i] = m_inputs[i - 1];
    }
    m_inputs[position] = input;

    run(get_time());
}

void node_t::remove_all_input(node_t* input) {
    for (size_t i = 0; i < m_inputs.size(); ++i) {
        if (m_inputs[i] == input) {
            erase_all_stable(m_inputs[i]->m_outputs, this);
            break ;
        }
    }

    if (erase_all_stable(m_inputs, input)) {
        run(get_time());
    }
}

void node_t::remove_input(node_t* input) {
    if (m_inputs.empty()) {
        return ;
    }

    bool erased = false;
    size_t p = 0;
    while (p < m_inputs.size()) {
        if (m_inputs[p] == input) {
            bool r = erase_stable(m_inputs[p]->m_outputs, this);
            assert(r);
            erased = true;
            break ;
        }

        ++p;
    }

    bool r = erase_stable(m_inputs, this);
    assert(r == erased);

    if (erased) {
        run(get_time());
    }
}

void node_t::isolate() {
    for (node_t* input : m_inputs) {
        erase_all_stable(input->m_outputs, this);
    }
    time_type_t t_propagate = get_time();
    for (node_t* output : m_outputs) {
        if (erase_all_stable(output->m_inputs, this)) {
            output->run(t_propagate);
        }
    }
    m_inputs.clear();
    m_outputs.clear();
}

void node_t::describe(string& str) {
    m_describe_fn(m_result, str);
}

bool node_t::run(time_type_t t_propagate) {
    if (!run_preamble(t_propagate)) {
        return false;
    }

    if (!run_impl()) {
        return false;
    }

    if (!run_postamble(t_propagate)) {
        return false;
    }

    return true;
}

bool node_t::run_force(time_type_t t_propagate) {
    if (!run_impl()) {
        return false;
    }

    if (!run_postamble(t_propagate)) {
        return false;
    }

    return true;
}

memory_slice_t& node_t::write() {
    return m_result;
}
