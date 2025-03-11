#include "state.h"

#include <stdexcept>

template <typename T, typename... Args>
T* state_t::add(const string& field_name, Args&&... args) {
    auto it = m.find(field_name);
    if (it != m.end()) {
        return 0;
    }
    it = m.insert({ field_name, { new T(std::forward<Args>(args)...), string(typeid(T).name()), [](void* data) { delete static_cast<T*>(data); } } }).first;
    return (T*) it->second.data;
}

template <typename T>
T* state_t::find(const string& field_name) {
    auto it = m.find(field_name);
    if (it == m.end()) {
        return 0;
        throw runtime_error("field does not exist");
    }
    return (T*) it->second.data;
}

template <typename T>
const T* state_t::find(const string& field_name) const {
    auto it = m.find(field_name);
    if (it == m.end()) {
        throw runtime_error("field does not exist");
    }
    return (const T*) it->second.data;
}

template <typename T>
const char* state_t::add_abstract(const string& field_name) {
    const char* type_name = typeid(T).name();
    auto it = abstract.find({ field_name, string(type_name) });
    if (it != abstract.end()) {
        throw runtime_error("field already exists");
    }
    it = abstract.insert({ field_name, string(type_name) }).first;
    return it->second.data();
}

template <typename T>
const char* state_t::find_abstract(const string& field_name) {
    const char* type_name = typeid(T).name();
    auto it = abstract.find({ field_name, string(type_name) });
    if (it == abstract.end()) {
        return 0;
    }
    return it->second.data();
}
