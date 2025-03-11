#include "state.h"

#include <cassert>
#include <stdexcept>

template <typename T, typename... Args>
T& state_t::add(const string& name, Args&&... args) {
    auto it = m.find(name);
    if (it != m.end()) {
        throw runtime_error("cannot add, name already exists");
    }
    auto r = m.insert({ name, { new T(std::forward<Args>(args)...), typeid(T).hash_code(), [](void* data) { delete (T*) data; } } });
    assert(r.second);
    it = r.first;
    return *(T*) it->second.data;
}

template <typename T>
void state_t::add_abstract(const string& name) {
    auto it = abstract_set.find({ name, typeid(T).hash_code() });
    if (it != abstract_set.end()) {
        throw runtime_error("cant add abstract as it already exists");
    }
    auto r = abstract_set.insert({ name, typeid(T).hash_code() });
    assert(r.second);
}

template <typename T>
const T& state_t::read(const string& name) const {
    auto it = m.find(name);
    if (it == m.end()) {
        throw runtime_error("cannot read '" + name + "', name does not exist");
    }
    if (it->second.type_hash != typeid(T).hash_code()) {
        throw runtime_error("cannot read '" + name + "', expecting different type");
    }
    return *(T*) it->second.data;
}

template <typename T>
T& state_t::write(const string& name) {
    auto it = m.find(name);
    if (it == m.end()) {
        throw runtime_error("cannot write '" + name + "', name does not exist");
    }
    if (it->second.type_hash != typeid(T).hash_code()) {
        throw runtime_error("cannot write '" + name + "', expecting different type");
    }
    return *(T*) it->second.data;
}

template <typename T>
const T* state_t::find(const string& name) const {
    auto it = m.find(name);
    if (it == m.end()) {
        return 0;
    }
    if (it->second.type_hash != typeid(T).hash_code()) {
        return 0;
    }
    return (const T*) it->second.data;
}

template <typename T>
T* state_t::find(const string& name) {
    auto it = m.find(name);
    if (it == m.end()) {
        return 0;
    }
    if (it->second.type_hash != typeid(T).hash_code()) {
        return 0;
    }
    return (T*) it->second.data;
}
