#include "state.h"

abstract_set_t::abstract_set_t():
    unordered_set<pair<string, size_t>, function<size_t(const pair<string, size_t>&)>, function<bool(const pair<string, size_t>&, const pair<string, size_t>&)>>(
        0,
        [](const pair<string, size_t>& p) {
            return hash<string>()(p.first) ^ hash<size_t>()(p.second);
        },
        [](const pair<string, size_t>& p1, const pair<string, size_t>& p2) {
            return p1 == p2;
        }

    )
{
}

state_t::~state_t() {
    for (const auto &p : m) {
        p.second.dtor(p.second.data);
    }
}

bool state_t::implements(const string& field_name, size_t type_hash) const {
    auto it = m.find(field_name);
    if (it == m.end()) {
        return false;
    }
    return type_hash == it->second.type_hash;
}
