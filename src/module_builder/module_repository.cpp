#include "module_repository.h"

void module_repository_t::add_module(module_t* module) {
    m_module_by_name.emplace(module->name(), module);
}

module_t* module_repository_t::find_module(const std::string& name) const {
    auto it = m_module_by_name.find(name);
    if (it != m_module_by_name.end()) {
        return it->second;
    }

    return nullptr;
}
