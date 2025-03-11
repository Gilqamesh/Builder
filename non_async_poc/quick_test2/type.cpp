#include "type.h"

#include <cassert>
#include <stdexcept>

type_t::type_t(
    const function<bool(state_t&)>& ctor,
    const function<void(state_t&)>& dtor,
    const function<bool(instance_t&)>& run
) {
    state.add<function<bool(state_t&)>>("ctor", ctor);
    state.add<function<void(state_t&)>>("dtor", dtor);
    state.add<function<bool(instance_t&)>>("run", run);
}

void type_t::inherit(type_t* type) {
    inputs.insert(type);
}

abstract_set_t type_t::collect_abstracts() const {
    abstract_set_t abstract_set;
    collect_abstracts(abstract_set);
    return abstract_set;
}

void type_t::add_coercion(type_t* to, const function<bool(const instance_t&, instance_t&)>& coercion_fn) {
    if (!coercion_fn) {
        throw runtime_error("coercion fn must be valid");
    }
    if (to == this) {
        throw runtime_error("can only coerce to different types");
    }
    auto it = coercions.find(to);
    if (it != coercions.end()) {
        throw runtime_error("coercion fn already exists for type");
    }
    coercions.insert({ to, coercion_fn });
}

void type_t::collect_abstracts(abstract_set_t& abstract_set) const {
    for (const auto& p : state.abstract_set) {
        auto r = abstract_set.insert(p);
        assert(r.second);
    }

    for (type_t* input : inputs) {
        input->collect_abstracts(abstract_set);
    }

    auto it = abstract_set.begin();
    while (it != abstract_set.end()) {
        if (state.implements(it->first, it->second)) {
            it = abstract_set.erase(it);
        } else {
            ++it;
        }
    }
}
