#include "module_builder.h"

#include <iostream>
#include <fstream>
#include <regex>
#include <format>

void module_builder_t::populate_dependencies(const module_repository_t& module_repository, module_t* module) {
    std::stack<module_t*> processed_stack;
    std::unordered_map<module_t*, dependency_population_state_t> dependency_population_state;
    populate_dependencies(module_repository, module, dependency_population_state, processed_stack);
}

void module_builder_t::compile_module(const artifact_repository_t& artifact_repository, module_t* module) {
}

void module_builder_t::populate_dependencies(
    const module_repository_t& module_repository,
    module_t* module,
    std::unordered_map<module_t*, dependency_population_state_t>& dependency_population_state,
    std::stack<module_t*>& processed_stack
) {
    auto dependency_population_state_it = dependency_population_state.find(module);
    if (dependency_population_state_it != dependency_population_state.end()) {
        if (dependency_population_state_it->second == dependency_population_state_t::POPULATED) {
            return ;
        }
        if (dependency_population_state_it->second == dependency_population_state_t::PROCESSING) {
            std::cerr << std::format("[error] cyclic module dependency detected between the following modules") << std::endl;
            while (!processed_stack.empty()) {
                module_t* module = processed_stack.top();
                processed_stack.pop();
                std::cerr << std::format("  {}", module->name()) << std::endl;
            }
            exit(1);
        }
    }
    processed_stack.emplace(module);

    std::ifstream ifs(module->module_cpp_path());
    if (!ifs) {
        std::cerr << std::format("[error] failed to open target module file: {}", module->module_cpp_path().string()) << std::endl;
        exit(1);
    }

    const std::regex import_regex(R"(^\s*#\s*include\s*["<](?:.*/)?([^.">/]+)(?:\.[^">/]+)?[">]\s*$)");
    std::string line;
    while (std::getline(ifs, line)) {
        std::smatch match;
        if (std::regex_match(line, match, import_regex)) {
            const std::string imported_module_name = match[1].str();
            if (imported_module_name != module->name()) {
                module_t* imported_module = module_repository.find_module(imported_module_name);
                if (imported_module != nullptr) {
                    populate_dependencies(module_repository, imported_module, dependency_population_state, processed_stack);
                    module->dependencies().emplace(imported_module_name, imported_module);
                }
            }
        }
    }

    dependency_population_state.emplace(module, dependency_population_state_t::PROCESSING);
    processed_stack.pop();
}
