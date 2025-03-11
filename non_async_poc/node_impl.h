#include "node.h"

template <typename signature_t>
signature_derived_t<signature_t>::signature_t add(const string& symbol_name, const signature_t& fn) {
    auto fn_pointer_it = m_symbol_str_to_fn_pointer.find(symbol_name);
    if (fn_pointer_it != m_symbol_str_to_fn_pointer.end()) {
        return 0;
    }
    fn_pointer_it = m_symbol_str_to_fn_pointer.insert({ symbol_name, make_shared<signature_t>(fn) }).first;
    return *static_pointer_cast<signature_t>(fn_pointer_it->second);
}

template <typename signature_t>
signature_derived_t<signature_t>::signature_t find(const string& symbol_name) {
    auto fn_pointer_it = m_symbol_str_to_fn_pointer.find(symbol_name);
    if (fn_pointer_it == m_symbol_str_to_fn_pointer.end()) {
        return 0;
    }
    return *static_pointer_cast<signature_t>(fn_pointer_it->second);
}

template <typename signature_t>
signature_t interface_t::add(const string& symbol_name, const signature_t& fn) {
    const char* signature_str = typeid(signature_t).name();
    auto signature_base_it = m_signature_str_to_signature_base.find(signature_str);
    if (signature_base_it == m_signature_str_to_signature_base.end()) {
        signature_base_it = m_signature_str_to_signature_base.insert({ signature_str, new signature_derived_t<signature_t> }).first;
    }
    return ((signature_derived_t<signature_t>*)(signature_base_it->second))->add(symbol_name, fn);
}

template <typename signature_t>
signature_t interface_t::find(const string& symbol_name) {
    const char* signature_str = typeid(signature_t).name();
    auto signature_base_it = m_signature_str_to_signature_base.find(signature_str);
    if (signature_base_it == m_signature_str_to_signature_base.end()) {
        return 0;
    }
    return ((signature_derived_t<signature_t>*)(signature_base_it->second))->find(symbol_name);
}

void interface_t::print(int depth) {
    cout << string(depth * 2, ' ') << "Interface:" << endl;
    for (const auto& p1 : m_signature_str_to_signature_base) {
        cout << string((depth + 1) * 2, ' ') << p1.first << ":" << endl;
        for (const auto& p2 : p1.second->m_symbol_str_to_fn_pointer) {
            cout << string((depth + 1) * 2, ' ') << "  " << p2.first << " -> " << p2.second << endl;
        }
    }
}

template <typename slot_t>
void node_t::add_slot() {
    const char* slot_name = typeid(slot_t).name();
    auto it = m_inputs.find(slot_name);
    if (it != m_inputs.end()) {
        throw runtime_error("slot already added");
    }
    m_inputs.insert({ slot_name, {} });
}

template <typename slot_t>
unordered_map<node_t*, size_t>& node_t::find_slot() {
    const char* slot_name = typeid(slot_t).name();
    auto it = m_inputs.find(slot_name);
    if (it == m_inputs.end()) {
        throw runtime_error("slot does not exist");
    }
    return it->second;
}

template <typename slot_t>
void node_t::add_input(node_t* input, size_t n) {
    if (n == 0) {
        return ;
    }

    // either input has interface or state which is of slot_t

    signature_t signature = input->find_interface<signature_t>();
    if (!signature) {
        cerr << "Input does not have signature for the slot" << endl;
        return ;
    }

    auto& slot = find_slot<signature_t>();

    auto input_it = slot.find(input);
    if (input_it == slot.end()) {
        auto r = slot.insert({ input, 0 });
        assert(r.second);
        input_it = r.first;
    }

    if (SIZE_MAX - n < input_it->second) {
        throw runtime_error("max number of inputs reached");
    } else {
        input_it->second += n;
    }

    auto output_it = m_outputs.find(this);
    if (output_it == m_outputs.end()) {
        auto r = m_outputs.insert({ this, 0 });
        assert(r.second);
        output_it = r.first;
    }

    assert(output_it->second <= SIZE_MAX - n);
    output_it->second += n;

    run(get_time());
}

template <typename slot_t>
void node_t::remove_input(node_t* input, size_t n) {
    if (n == 0) {
        return ;
    }

    auto& slot = find_slot<slot_t>();

    auto input_it = slot.find(input);
    if (input_it == slot.end()) {
        return ;
    }

    if (input_it->second <= n) {
        input_it->first->m_outputs.erase(this);
        slot.erase(input_it);
    } else {
        input_it->second -= n;
        auto output_it = input_it->first->m_outputs.find(this);
        assert(output_it != input_it->first->m_outputs.end() && n < output_it->second);
        output_it->second -= n;
    }

    run(get_time());
}

template <typename signature_t>
signature_t node_t::find_interface(const string& symbol_name) {
    return m_interface.find<signature_t>(symbol_name);
}

template <typename T>
T* node_t::add_state() {
    const char* state_name = typeid(T).name()
    auto it = m_states.find(state_name);
    if (it != m_states.end()) {
        throw runtime_error("state already exists");
    }
    state_t state;
    state.size = sizeof(T);
    state.memory = (void*) (new T);
    m_states.insert({ state_name, state });
    return (T*)state.memory;
}

template <typename T>
void node_t::remove_state() {
    const char* state_name = typeid(T).name()
    auto it = m_states.find(state_name);
    if (it == m_states.end()) {
        throw runtime_error("state does not exist");
    }
    assert(it->second.memory);
    delete (T*)(it->second.memory);
    m_states.erase(it);
}

template <typename T>
const T* node_t::read_state() const {
    const char* state_name = typeid(T).name()
    auto it = m_states.find(state_name);
    if (it == m_states.end()) {
        return 0;
    }
    return (const T*)it->second.memory;
}

template <typename T>
T* node_t::write_state() {
    const char* state_name = typeid(T).name()
    auto it = m_states.find(state_name);
    if (it == m_states.end()) {
        return 0;
    }
    return (T*)it->second.memory;
}

template <typename node_type_t>
node_type_t* find_or_create() {
    extern unordered_map<string, node_t*> node_database;
    const char* node_name = typeid(node_type_t).name()
    auto it = node_database.find(node_name);
    if (it != node_database.end()) {
        return it->second;
    }
    auto r = node_database.insert({ node_name, new node_type_t });
    assert(r.second);
    it = r.first;
    assert(it->second);
    return (node_type_t*)it->second;
}
