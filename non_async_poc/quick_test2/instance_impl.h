#include "instance.h"

template <typename T>
const T* instance_t::find(const string& name) const {
    unordered_set<type_t*> seen;
    return find<T>(name, m_type, seen);
}

template <typename T>
T* instance_t::find(const string& name) {
    unordered_set<type_t*> seen;
    return find<T>(name, m_type, seen);
}

template <typename T>
const T& instance_t::read(const string& name) const {
    const T* result = find<T>(name);
    if (!result) {
        throw runtime_error("cannot read '" + name + "', it does not exist");
    }
    return *result;
}

template <typename T>
T& instance_t::write(const string& name) {
    T* result = find<T>(name);
    if (!result) {
        throw runtime_error("cannot write '" + name + "', it does not exist");
    }
    return *result;
}

template <typename T>
const T* instance_t::find(const string& name, type_t* type, unordered_set<type_t*>& seen) const {
    if (seen.find(type) != seen.end()) {
        return 0;
    }
    seen.insert(type);

    auto it = m_states.find(type);
    assert(it != m_states.end());
    const T* result = it->second.find<T>(name);
    if (result) {
        return result;
    }
    result = type->state.find<T>(name);
    if (result) {
        return result;
    }

    for (type_t* input : type->inputs) {
        result = find<T>(name, input, seen);
        if (result) {
            return result;
        }
    }

    return 0;
}

template <typename T>
T* instance_t::find(const string& name, type_t* type, unordered_set<type_t*>& seen) {
    if (seen.find(type) != seen.end()) {
        return 0;
    }
    seen.insert(type);

    auto it = m_states.find(type);
    assert(it != m_states.end());
    T* result = it->second.find<T>(name);
    if (result) {
        return result;
    }
    result = type->state.find<T>(name);
    if (result) {
        return result;
    }

    for (type_t* input : type->inputs) {
        result = find<T>(name, input, seen);
        if (result) {
            return result;
        }
    }

    return 0;
}
