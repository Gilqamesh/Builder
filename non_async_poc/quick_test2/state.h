#ifndef STATE_H
# define STATE_H

# include <unordered_set>
# include <unordered_map>
# include <string>
# include <functional>

using namespace std;

struct abstract_set_t : public unordered_set<pair<string, size_t>, function<size_t(const pair<string, size_t>&)>, function<bool(const pair<string, size_t>&, const pair<string, size_t>&)>> {
    abstract_set_t();
};

struct state_t {
    struct entry_t {
        void* data;
        size_t type_hash;
        function<void(void*)> dtor;
    };
    unordered_map<string, entry_t> m;
    abstract_set_t abstract_set;

    state_t() = default;
    ~state_t();

    state_t(const state_t& other) = delete;
    state_t& operator=(const state_t& other) = delete;

    state_t(state_t&& other) noexcept = default;
    state_t& operator=(state_t&& other) = default;


    bool implements(const string& field_name, size_t type_hash) const;

    template <typename T, typename... Args>
    T& add(const string& name, Args&&... args);

    template <typename T>
    void add_abstract(const string& name);

    template <typename T>
    const T& read(const string& name) const;

    template <typename T>
    T& write(const string& name);

    template <typename T>
    const T* find(const string& name) const;

    template <typename T>
    T* find(const string& name);
};

# include "state_impl.h"

#endif // STATE_H
