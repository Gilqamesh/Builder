#ifndef DATA_H
# define DATA_H

# include "state.h"
# include "type.h"

using namespace std;

struct data_t {
    state_t state;
    type_t* type;

    data_t(type_t* type);

    template <typename T, typename... Args>
    T& add_state(const string& field_name, Args&&... args);

    // aka write state
    template <typename T>
    T& find_state(const string& field_name);

    // aka read state
    template <typename T>
    const T& find_state(const string& field_name) const;

    template <typename T>
    const T* find_assertion(const string& assertion_name) const;

    bool convert(data_t& result) const;
};

# include "data_impl.h"

#endif // DATA_H
