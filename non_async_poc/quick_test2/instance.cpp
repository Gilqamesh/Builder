#include "instance.h"

#include "type.h"

#include <stdexcept>
#include <iostream>
#include <cassert>
#include <queue>

static time_type_t t_init;

void set_time_init() {
    t_init = get_time();
}

time_type_t get_time() {
    return chrono::high_resolution_clock::now();
}

time_type_t get_time_init() {
    return t_init;
}

instance_t::instance_t(type_t* type): m_type(type) {
    abstract_set_t abstract_set = type->collect_abstracts();
    if (!abstract_set.empty()) {
        cout << "unimplemented abstracts:" << endl;
        for (const auto& p : abstract_set) {
            cout << "  " << p.first << endl;
        }
        throw runtime_error("cannot instance abstract type");
    }

    if (!instantiate()) {
        throw runtime_error("could not instantiate");
    }
};

instance_t::~instance_t() {
    destruct();
}

optional<instance_t> instance_t::convert(type_t* type) const {
    instance_t result(type);
    auto it = m_type->coercions.find(result.m_type);
    if (it != m_type->coercions.end() && it->second(*this, result)) {
        return result;
    }

    queue<type_t*> q;
    for (const auto &p : m_type->coercions) {
        p.first->parent = m_type;
        q.push(p.first);
    }

    while (!q.empty()) {
        type_t* type = q.front();
        q.pop();
        if (type == result.m_type) {
            if (convert(type, *this, result)) {
                return result;
            }
        }
        for (const auto& p : type->coercions) {
            type_t* old_parent = p.first->parent;
            p.first->parent = type;
            unordered_set<type_t*> seen;
            type_t* cur = p.first;
            bool has_circle = false;
            while (cur) {
                auto it = seen.find(cur);
                if (it != seen.end()) {
                    has_circle = true;
                    break ;
                }
                seen.insert(cur);
                cur = cur->parent;
            }

            if (has_circle) {
                p.first->parent = old_parent;
            } else {
                q.push(p.first);
            }
        }
    }

    return {};
}

void instance_t::add_input(const string& slot_name, instance_t* input, size_t n) {
    auto slot_it = m_inputs.find(slot_name);
    if (slot_it == m_inputs.end()) {
        throw runtime_error("could not add input, slot name does not exist");
    }

    auto input_it = slot_it->second.find(input);
    if (input_it == slot_it->second.end()) {
        auto r = it->second.insert({ input, 0 });
        assert(r.second);
        input_it = r.first;
    }
    assert(input_it->second < SIZE_MAX - n);
    input_it->second += n;

    auto self_it = input->m_outputs.find(this);
    if (self_it == input->m_outputs.end()) {
        auto r = input->m_outputs.insert({ this, 0 });
        assert(r.second);
        self_it = r.first;
    }
    assert(self_it->second < SIZE_MAX - n);
    self_it->second += n;
}

void instance_t::remove_input(const string& slot_name, instance_t* input, size_t n) {
    auto slot_it = m_inputs.find(slot_name);
    if (slot_it == m_inputs.end()) {
        throw runtime_error("could not remove input, slot name does not exist");
    }

    auto input_it = slot_it->second.find(input);
    if (input_it == slot_it->second.end()) {
        throw runtime_error("could not remove input as it wasn't found");
    }

    if (n < input_it->second) {
        input_it->second -= n;
        auto self_it = input->m_outputs.find(this);
        assert(self_it != input->m_outputs.end());
        assert(n < self_it->second);
        self_it->second -= n;
    } else {
        input->m_outputs.erase(this);
        slot_it->second.erase(input_it);
    }
}

void instance_t::isolate() {
    for (auto& p : m_inputs) {
        for (auto &p2 : p.second) {
            p2.first->m_outputs.erase(this);
        }
        p.second.clear();
    }
    for (auto &p : m_outputs) {
        auto it = p.first->m_inputs.find(m_type);
        assert(it != p.first->m_inputs.end());
        it->second.erase(this);
    }
    m_outputs.clear();
}

bool instance_t::run(time_type_t t_propagate) {
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

bool instance_t::run_force(time_type_t t_propagate) {
    if (!run_impl()) {
        return false;
    }

    if (!run_postamble(t_propagate)) {
        return false;
    }

    return true;
}

bool instance_t::instantiate(type_t* type, unordered_set<type_t*>& seen) {
    if (seen.find(type) != seen.end()) {
        return true;
    }
    seen.insert(type);

    for (type_t* input : type->inputs) {
        if (!instantiate(input, seen)) {
            return false;
        }
    }

    auto r = m_states.emplace(type, state_t());
    assert(r.second);
    auto it = r.first;
    auto ctor = type->state.find<function<bool(state_t&)>>("ctor");
    assert(ctor && (*ctor));
    return (*ctor)(it->second);
}

bool instance_t::instantiate() {
    unordered_set<type_t*> seen;
    return instantiate(m_type, seen);
}

void instance_t::destruct(type_t* type, unordered_set<type_t*>& seen) {
    if (seen.find(type) != seen.end()) {
        return ;
    }
    seen.insert(type);

    auto it = m_states.find(type);
    assert(it != m_states.end());
    auto dtor = type->state.find<function<void(state_t&)>>("dtor");
    assert(dtor && (*dtor));
    (*dtor)(it->second);
    m_states.erase(it);

    for (type_t* input : type->inputs) {
        destruct(input, seen);
    }
}

void instance_t::destruct() {
    if (m_states.empty()) {
        // invalid state after move
        return ;
    }

    unordered_set<type_t*> seen;
    destruct(m_type, seen);
}

bool instance_t::convert(type_t* to, const instance_t& prev, instance_t& result) const {
    if (to->parent == m_type) {
        auto it = m_type->coercions.find(to);
        assert(it != m_type->coercions.end());
        return it->second(prev, result);
    }

    instance_t subresult(to->parent);
    if (convert(to->parent, prev, subresult)) {
        auto it = to->parent->coercions.find(to);
        assert(it != to->parent->coercions.end());
        return it->second(subresult, result);
    } else {
        return false;
    }
}

bool instance_t::run_preamble(time_type_t t_propagate) {
    if (t_propagate < m_t_propagate) {
        return false;
    }

    function<void(instance_t* instance)> propagate_time;
    propagate_time = [&propagate_time, t_propagate](instance_t* instance) {
        if (t_propagate <= instance->m_t_propagate) {
            return ;
        }
        instance->m_t_propagate = t_propagate;

        for (const auto& output_pair : instance->m_outputs) {
            propagate_time(output_pair.first);
        }
    };
    propagate_time(this);

    if (m_inputs.empty()) {
        return true;
    }

    time_type_t most_recent_input;
    for (const auto& input_p : m_inputs) {
        for (const auto& p : input_p.second) {
            if (p.first->m_t_propagate == t_propagate) {
                if (p.first->m_t_start < t_propagate) {
                    return false;
                }
            }
            if (p.first->m_t_success < p.first->m_t_failure) {
                return false;
            }
            most_recent_input = max(most_recent_input, p.first->m_t_success);
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

bool instance_t::run_impl() {
    m_t_start = get_time();
    auto run_fn = m_type->state.read<function<bool(instance_t&)>>("run");
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

bool instance_t::run_postamble(time_type_t t_propagate) {
    // todo: use queue to incorporate looping constructs, otherwise stack blows up

    for (const auto& output_pair : m_outputs) {
        if (output_pair.first->m_t_start < m_t_success) {
            (void) output_pair.first->run(t_propagate);
        }
    }

    return true;
}
