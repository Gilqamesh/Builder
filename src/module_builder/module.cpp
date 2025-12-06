#include "module.h"

#include <iostream>
#include <imgui.h>

module_t::module_t(const std::string& name, const std::filesystem::path module_cpp_path):
    m_name(name),
    m_module_cpp_path(module_cpp_path)
{
}

const std::string& module_t::name() const {
    return m_name;
}

const std::filesystem::path& module_t::module_cpp_path() const {
    return m_module_cpp_path;
}

std::unordered_map<std::string, module_t*>& module_t::dependencies() {
    return m_dependencies;
}

void module_t::print_dependencies() const {
    std::cout << "[dependencies]" << std::endl;
    print_dependencies(0);
}

void module_t::print_dependencies(int depth) const {
    for (int i = 0; i < depth; ++i) {
        std::cout << "  ";
    }
    std::cout << m_name << std::endl;

    for (const auto& [dependency_name, dependency_module] : m_dependencies) {
        (void) dependency_name;
        dependency_module->print_dependencies(depth + 1);
    }
}
