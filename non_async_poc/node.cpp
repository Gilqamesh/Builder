#include "node.h"

# include <algorithm>
# include <iostream>
# include <cassert>

#define LINE() cout << __LINE__ << endl

unordered_map<string, node_t*> node_database;

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

static int n_nodes = 0;
node_t::node_t(
    const function<bool(node_t&)>& constructor_fn,
    const function<bool(node_t&)>& run_fn,
    const function<string(const node_t&)>& describe_fn,
    const function<void(node_t&)>& destructor_fn
) {
    if (n_nodes++ == 0) {
        set_time_init();
    }

    m_interface.add<function<bool(node_t&)>>("constructor", constructor_fn); // called once..
    m_interface.add<function<bool(node_t&)>>("run", run_fn);
    m_interface.add<function<string(const node_t&)>>("describe", describe_fn);
    m_interface.add<function<void(node_t&)>>("destructor", destructor_fn); // called once..

    if (!constructor_fn(*this)) {
        throw runtime_error("constructor failed");
    }
}

node_t::~node_t() {
    isolate();
    auto destructor_fn = m_interface.find<function<void(node_t&)>>("destructor");
    assert(destructor_fn);
    destructor_fn(*this);

    assert(n_nodes--);
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

        for (const auto& output_pair : node->m_outputs) {
            propagate_time(output_pair.first);
        }
    };
    propagate_time(this);

    if (m_inputs.empty()) {
        return true;
    }

    time_type_t most_recent_input;
    for (const auto& input_slot_pair : m_inputs) {
        for (const auto& input_pair : input_slot_pair.second) {
            if (input_pair.first->m_t_propagate == t_propagate) {
                if (input_pair.first->m_t_start < t_propagate) {
                    return false;
                }
            }
            if (input_pair.first->m_t_success < input_pair.first->m_t_failure) {
                return false;
            }
            most_recent_input = max(most_recent_input, input_pair.first->m_t_success);
        }
    }

    if (most_recent_input <= m_t_success) {
        return false;
    }

    if (m_outputs.empty()) {
        return true;
    }

    time_type_t most_oldest_output = get_time();
    for (const auto& output_pair : m_outputs) {
        most_oldest_output = min(most_oldest_output, output_pair.first->m_t_success);
    }

    if (most_recent_input <= most_oldest_output) {
        return false;
    }

    return true;
}

bool node_t::run_impl() {
    m_t_start = get_time();
    auto run_fn = m_interface.find<function<bool(node_t&)>>("run");
    assert(run_fn);
    if (!run_fn(*this)) {
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

    for (const auto& output_pair : m_outputs) {
        if (output_pair.first->m_t_start < m_t_success) {
            (void) output_pair.first->run(t_propagate);
        }
    }

    return true;
}

void node_t::isolate() {
    for (const auto& input_slot_pair : m_inputs) {
        for (const auto& input_pair : input_slot_pair.second) {
            input_pair.first->m_outputs.erase(this);
        }
        assert(input_slot_pair.second.empty());
    }

    time_type_t t_propagate = get_time();
    for (const auto& output_pair : m_outputs) {
        for (auto& input_slot_pair : output_pair.first->m_inputs) {
            input_slot_pair.second.erase(this);
        }
        output_pair.first->run(t_propagate);
    }
}

string node_t::describe() {
    auto describe_fn = m_interface.find<function<string(node_t&)>>("describe");
    assert(describe_fn);
    return describe_fn(*this);
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

const decltype(node_t::m_inputs)& node_t::read_inputs() const {
    // todo: no write to inputs while reading
    return m_inputs;
}


decltype(node_t::m_inputs)& node_t::write_inputs() {
    return m_inputs;
}
