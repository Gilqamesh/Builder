#ifndef STATE_H
# define STATE_H

# include <functional>
# include <unordered_map>
# include <unordered_set>
# include <string>

using namespace std;

struct state_t {
    struct m_entry_t {
        void* data;
        string type_name;
        function<void(void*)> dtor;
    };

    // unordered_map<string, m_entry_t> immutable;
    // unordered_map<string, m_entry_t> transient;

    unordered_map<string, m_entry_t> m;
    unordered_set<pair<string, string>, function<size_t(const pair<string, string>&)>, function<bool(const pair<string, string>&, const pair<string, string>&)>> abstract;

    state_t();
    ~state_t();

    state_t(const state_t& other) = delete;
    state_t& operator=(const state_t& other) = delete;

    state_t(state_t&& other) noexcept = default;
    state_t& operator=(state_t&& other) noexcept = default;

    template <typename T, typename... Args>
    T* add(const string& field_name, Args&&... args);

    template <typename T>
    T* find(const string& field_name);

    template <typename T>
    const T* find(const string& field_name) const;

    // note: if we do this, all output will be abstract until they implement this method..
    template <typename T>
    const char* add_abstract(const string& field_name);

    template <typename T>
    const char* find_abstract(const string& field_name);

    const decltype(abstract)& get_abstract() const;

    bool implements(const string& field_name, const string& type_name) const;
};

# include "state_impl.h"

#endif // STATE_H
