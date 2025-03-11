#include "type.h"

template <typename T, typename... Args>
const T& type_t::add(const string& field_name, Args&&... args) {
    T* result = interface.add<T>(field_name, std::forward<Args>(args)...);
    if (!result) {
        throw runtime_error("field already exists");
    }
    return *result;
}

template <typename T>
const T* type_t::find(const string& field_name) const {
    const T* result = interface.find<T>(field_name);
    if (result) {
        return result;
    }
    for (type_t* input : inputs) {
        result = input->find<T>(field_name);
        if (result) {
            // convert to input
            return result;
        }
    }
    
    return 0;
}

template <typename T>
const char* type_t::add_abstract(const string& field_name) {
    return interface.add_abstract<T>(field_name);
}

template <typename T>
const char* type_t::find_abstract(const string& field_name) {
    return interface.find_abstract<T>(field_name);
}
