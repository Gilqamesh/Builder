#include "type.h"

template <typename T>
const T& type_t::read(const string& name) const {
    return state.read<T>(name);
}

template <typename T>
T& type_t::write(const string& name) {
    return state.write<T>(name);
}

template <typename T, typename... Args>
T& type_t::add(const string& name, Args&&... args) {
    return state.add<T>(name, std::forward<Args>(args)...);
}

template <typename T>
void type_t::add_abstract(const string& name) {
    return state.add_abstract<T>(name);
}

template <typename T>
const T* type_t::find(const string& name) const {
    return state.find<T>(name);
}

template <typename T>
T* type_t::find(const string& name) {
    return state.find<T>(name);
}
