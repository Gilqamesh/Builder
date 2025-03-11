#include "state.h"

state_t::state_t():
    abstract(
        0,
        [](const pair<string, string>& p) {
            return hash<string>()(p.first) ^ hash<string>()(p.second);
        },
        [](const pair<string, string>& p1, const pair<string, string>& p2) {
            return p1 == p2;
        }
    )
{
}

state_t::~state_t() {
    for (auto& p : m) {
        p.second.dtor(p.second.data);
    }
}

const decltype(state_t::abstract)& state_t::get_abstract() const {
    return abstract;
}

bool state_t::implements(const string& field_name, const string& type_name) const {
    auto it = m.find(field_name);
    if (it == m.end()) {
        return false;
    }
    return type_name == it->second.type_name;
}
