#ifndef PRIMITIVE_REPOSITORY_H
# define PRIMITIVE_REPOSITORY_H

# include "libc.h"
# include "function_id.h"

// -----------------------------------------------------------------------------
// B — registry
// -----------------------------------------------------------------------------
struct B {
    std::unordered_map<function_id_t, std::function<void(function_t*)>> primitives;

    template <typename Func>
    void register_primitive(const std::string& name, Func&& f) {
        using traits    = function_traits<std::decay_t<Func>>;
        using signature = typename traits::signature;

        primitives[name] = [f = std::forward<Func>(f)](function_t* ft) mutable {
            primitive_wrapper<signature>::call(ft, f);
        };
    }
};

#endif // PRIMITIVE_REPOSITORY_H
