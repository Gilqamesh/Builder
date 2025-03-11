#include "data.h"

template <typename T, typename... Args>
T& data_t::add_state(const string& field_name, Args&&... args) {
    T* result = state.add<T>(field_name, std::forward<Args>(args)...);
    if (!result) {
        throw runtime_error("state already exists");
    }
    return *result;
}

template <typename T>
T& data_t::find_state(const string& field_name) {
    T* result = state.find<T>(field_name);
    if (!result) {
        throw runtime_error("state does not exist");
    }
    return *result;
}

template <typename T>
const T& data_t::find_state(const string& field_name) const {
    const T* result = state.find<T>(field_name);
    if (!result) {
        throw runtime_error("state does not exist");
    }
    return *result;
}

template <typename T>
const T* data_t::find_assertion(const string& assertion_name) const {
    const T* result = type->find<T>(assertion_name);
}
