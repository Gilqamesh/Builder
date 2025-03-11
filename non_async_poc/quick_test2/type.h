#ifndef TYPE_T
# define TYPE_T

# include "state.h"

struct instance_t;

struct type_t {
    state_t state;
    unordered_set<type_t*> inputs;
    unordered_map<type_t*, function<bool(const instance_t&, instance_t&)>> coercions;

    // used for bfs path traverse
    type_t* parent;

    type_t(
        const function<bool(state_t&)>& ctor,
        const function<void(state_t&)>& dtor,
        const function<bool(instance_t&)>& run
    );

    void inherit(type_t* type);

    abstract_set_t collect_abstracts() const;

    void add_coercion(type_t* to, const function<bool(const instance_t&, instance_t&)>& coercion_fn);

    template <typename T>
    const T& read(const string& name) const;

    template <typename T>
    T& write(const string& name);

    template <typename T, typename... Args>
    T& add(const string& name, Args&&... args);

    template <typename T>
    void add_abstract(const string& name);

    template <typename T>
    const T* find(const string& name) const;

    template <typename T>
    T* find(const string& name);

private:
    void collect_abstracts(abstract_set_t& abstract_set) const;
};

# include "type_impl.h"

#endif // TYPE_T
