#include "type.h"

#include "data.h"

#include <iostream>
#include <cassert>

type_t::type_t(const string& name): name(name) {
}

void type_t::add_input(type_t* input) {
    if (!input->is_abstract()) {
        throw runtime_error("input is not abstract, converter function must be supplied");
    }

    auto it = inputs.find(input);
    if (it != inputs.end()) {
        return ;
    }

    inputs.insert(input);
    input->outputs.insert(this);

    unordered_set<const type_t*> s;
    if (has_cycle(s, input)) {
        inputs.erase(input);
        input->outputs.erase(this);
        throw runtime_error("detected circular dependency");
    }
}

void type_t::add_input(type_t* input, const function<data_t(const data_t&)>& convert_to_input_fn) {
    auto it = inputs.find(input);
    if (it != inputs.end()) {
        return ;
    }

    add_convert_to(input, [convert_to_input_fn](const data_t& data, data_t& result) {
        result = convert_to_input_fn(data);
        return true;
    });

    inputs.insert(input);
    input->outputs.insert(this);

    unordered_set<const type_t*> s;
    if (has_cycle(s, input)) {
        inputs.erase(input);
        input->outputs.erase(this);
        throw runtime_error("detected circular dependency");
    }
}

void type_t::print() const {
    cout << "Type hierarchy of '" << name << "':" << endl;
    print(this);

    cout << "Type coercion of: '" << name << "':" << endl;
    unordered_set<const type_t*> visited;
    print_coercion(this, visited);
}

void type_t::add_convert_to(type_t* to, const function<bool(const data_t&, data_t&)>& convert_to_fn) {
    if (!convert_to_fn) {
        throw runtime_error("converter fn must be valid");
    }
    if (to == this) {
        throw runtime_error("can only convert to a different type");
    }
    auto it = convert_to_fns.find(to);
    if (it != convert_to_fns.end()) {
        throw runtime_error("converter fn already exists");
    }
    convert_to_fns.insert({ to, convert_to_fn });
}

bool type_t::is_abstract() const {
    // if type has any abstract things in its interface, then its abstract automatically..
    if (!interface.abstract.empty()) {
        return true;
    }

    unordered_set<pair<string, string>, function<size_t(const pair<string, string>&)>, function<bool(const pair<string, string>&, const pair<string, string>&)>> s(
        0,
        [](const pair<string, string>& p) {
            return hash<string>()(p.first) ^ hash<string>()(p.second);
        },
        [](const pair<string, string>& p1, const pair<string, string>& p2) {
            return p1 == p2;
        }
    );
    return is_abstract(s);
}

void type_t::print(const type_t* type, int depth) const {
    cout << string(depth * 4, ' ') << type->name << endl;
    for (const type_t* input : type->inputs) {
        print(input, depth + 1);
    }
}

void type_t::print_coercion(const type_t* type, unordered_set<const type_t*>& visited, int depth) const {
    auto it = visited.find(type);
    if (it != visited.end()) {
        return ;
    }
    visited.insert(type);
    for (const auto& p : type->convert_to_fns) {
        cout << string(depth * 2, ' ') << type->name << " -> " << p.first->name << endl;
        print_coercion(p.first, visited, depth + 1);
    }
    visited.erase(type);
}

bool type_t::has_cycle(unordered_set<const type_t*>& s, type_t* base) const {
    auto it = s.find(base);
    if (it != s.end()) {
        return true;
    }

    s.insert(base);
    for (type_t* input : base->inputs) {
        if (has_cycle(s, input)) {
            return true;
        }
    }
    s.erase(base);

    return false;
}

bool type_t::is_abstract(unordered_set<pair<string, string>, function<size_t(const pair<string, string>&)>, function<bool(const pair<string, string>&, const pair<string, string>&)>>& s) const {
    // add our own abstract things..
    for (const auto& p : interface.get_abstract()) {
        auto r = s.insert(p);
        assert(r.second);
    }

    // go over all inputs.. and collect the unimplemented things..
    for (type_t* input : inputs) {
        input->is_abstract(s);
    }

    // cross out any stuff which we implemented
    auto it = s.begin();
    while (it != s.end()) {
        if (interface.implements(it->first, it->second)) {
            it = s.erase(it);
        } else {
            ++it;
        }
    }

    // otherwise check if this type implements all abstract things from its inputs..
    // what does this mean?..
    // it means that if there are any unimplemented thing found anywhere from the inputs, then this type is abstract
    return !s.empty();
}
