#ifndef TYPE_H
# define TYPE_H

# include "state.h"

# include <unordered_set>
# include <string>

using namespace std;

struct data_t;

struct type_t {
    string name;

    state_t interface;

    unordered_set<type_t*> inputs;
    unordered_set<type_t*> outputs;
    unordered_map<type_t*, function<bool(const data_t&, data_t&)>> convert_to_fns;

    type_t(const string& name);

    void add_input(type_t* input);
    void add_input(type_t* input, const function<data_t(const data_t&)>& convert_to_input_fn);

    void print() const;

    void add_convert_to(type_t* to, const function<bool(const data_t&, data_t&)>& convert_to_fn);

    bool is_abstract() const;

    template <typename T, typename... Args>
    const T& add(const string& field_name, Args&&... args);

    template <typename T>
    const T* find(const string& field_name) const;

    template <typename T>
    const char* add_abstract(const string& field_name);

    template <typename T>
    const char* find_abstract(const string& field_name);

private:
    void print(const type_t* type, int depth = 0) const;
    void print_coercion(const type_t* type, unordered_set<const type_t*>& visited, int depth = 0) const;
    bool has_cycle(unordered_set<const type_t*>& s, type_t* base) const;
    bool is_abstract(unordered_set<pair<string, string>, function<size_t(const pair<string, string>&)>, function<bool(const pair<string, string>&, const pair<string, string>&)>>& s) const;
};

# include "type_impl.h"

#endif // TYPE_H
