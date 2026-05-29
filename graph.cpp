#include "graph.h"

#include "external/json.hpp"
#include "phase.h"

#include <fstream>
#include <stack>
#include <cassert>
#include <type_traits>

namespace kernel {

namespace cpp_builder {

namespace graph {

filesystem::path_t module_t::source_dir() const {
    return workspace->workspace_ecosystem->absolute_path_to_workspace_directory / workspace->workspace_relative_path_to_workspace_ecosystem / module_relative_path_to_workspace;
}

filesystem::path_t module_t::builder_path() const {
    return source_dir() / filesystem::relative_path_t(BUILDER_CPP);
}

filesystem::path_t module_t::artifact_base_dir() const {
    return workspace->workspace_ecosystem->artifact_dir / workspace->workspace_relative_path_to_workspace_ecosystem / module_relative_path_to_workspace;
}

filesystem::path_t module_t::artifact_dir() const {
    const auto versioned_dir_name = std::format("{}@{}", module_relative_path_to_workspace.to_native_path().filename().string(), version.value);
    return artifact_base_dir() / filesystem::relative_path_t(versioned_dir_name);
}

filesystem::path_t module_t::artifact_latest_dir() const {
    return artifact_base_dir() / filesystem::relative_path_t("latest");
}

filesystem::path_t module_t::builder_dir() const {
    return artifact_dir() / filesystem::relative_path_t("builder");
}

filesystem::path_t module_t::builder_build_dir() const {
    return builder_dir() / filesystem::relative_path_t("build");
}

filesystem::path_t module_t::builder_install_dir() const {
    return builder_dir() / filesystem::relative_path_t("install");
}

filesystem::path_t module_t::builder_install_path() const {
    return builder_install_dir() / filesystem::relative_path_t("builder.so");
}

filesystem::path_t module_t::builder_install_latest_path() const {
    return artifact_latest_dir() / filesystem::relative_path_t("builder/install/builder.so");
}

builder::module_phases_t& module_t::phases(builder::library_type_t library_type) const {
    switch (library_type) {
        case builder::library_type_t::STATIC:
            if (static_phases == nullptr) {
                static_phases = new builder::module_phases_t(const_cast<module_t&>(*this), library_type);
            }
            return *static_phases;
        case builder::library_type_t::SHARED:
            if (shared_phases == nullptr) {
                shared_phases = new builder::module_phases_t(const_cast<module_t&>(*this), library_type);
            }
            return *shared_phases;
        default:
            throw std::runtime_error(std::format("kernel::cpp_builder::graph::module_t::phases: unknown library_type {}", static_cast<std::underlying_type_t<builder::library_type_t>>(library_type)));
    }
}

builder::config_phase_t& module_t::config_phase(builder::library_type_t library_type) const {
    return phases(library_type).config;
}

builder::source_phase_t& module_t::source_phase(builder::library_type_t library_type) const {
    return phases(library_type).source;
}

builder::interface_phase_t& module_t::interface_phase(builder::library_type_t library_type) const {
    return phases(library_type).interface;
}

builder::library_phase_t& module_t::library_phase(builder::library_type_t library_type) const {
    return phases(library_type).library;
}

builder::binary_phase_t& module_t::binary_phase(builder::library_type_t library_type) const {
    return phases(library_type).binary;
}

version_t derive_version(const std::filesystem::file_time_type& file_time_type) {
    return version_t {
        .value = static_cast<uint64_t>(file_time_type.time_since_epoch().count() - std::numeric_limits<std::filesystem::file_time_type::duration::rep>::min())
    };
}

version_t derive_version(const filesystem::path_t& directory) {
    auto latest_module_file = filesystem::last_write_time(directory);

    for (const auto& path : filesystem::find(directory, filesystem::find_include_predicate_t::include_all, filesystem::find_descend_predicate_t::descend_all)) {
        latest_module_file = std::max(latest_module_file, filesystem::last_write_time(path));
    }

    return derive_version(latest_module_file);
}

module_t* workspace_ecosystem_t::discover_module_impl(filesystem::relative_path_t workspace_relative_path_to_workspace_ecosystem, filesystem::relative_path_t module_relative_path_to_workspace) {
    auto workspace_it = workspace_by_workspace_relative_path_to_workspace_ecosystem.find(workspace_relative_path_to_workspace_ecosystem);
    if (workspace_it == workspace_by_workspace_relative_path_to_workspace_ecosystem.end()) {
        json_workspace_t json_workspace;
        {
            const auto workspace_json_file = absolute_path_to_workspace_directory / workspace_relative_path_to_workspace_ecosystem / filesystem::relative_path_t(WORKSPACE_JSON);
            if (!filesystem::exists(workspace_json_file)) {
                throw std::runtime_error(std::format("kernel::cpp_builder::graph::discover_module_impl: file does not exist: '{}'", workspace_json_file.string()));
            }

            std::ifstream ifs(workspace_json_file.string());
            if (!ifs) {
                throw std::runtime_error(std::format("kernel::cpp_builder::graph::discover_module_impl: failed to open file '{}'", workspace_json_file.string()));
            }

            try {
                nlohmann::json json = nlohmann::json::parse(ifs);
                json_workspace = json.get<decltype(json_workspace)>();
            } catch (const nlohmann::json::parse_error& e) {
                throw std::runtime_error(std::format("kernel::cpp_builder::graph::discover_module_impl: failed to parse JSON file '{}': {}", workspace_json_file.string(), e.what()));
            } catch (const nlohmann::json::exception& e) {
                throw std::runtime_error(std::format("kernel::cpp_builder::graph::discover_module_impl: failed to get JSON workspace from file '{}': {}", workspace_json_file.string(), e.what()));
            }
        }

        auto workspace = new workspace_t {
            .workspace_ecosystem = this,
            .workspace_relative_path_to_workspace_ecosystem = workspace_relative_path_to_workspace_ecosystem,
            .level = json_workspace.level
        };
        workspace_it = workspace_by_workspace_relative_path_to_workspace_ecosystem.emplace(workspace_relative_path_to_workspace_ecosystem, workspace).first;
    }
    auto workspace = workspace_it->second;
    auto module_it = workspace->module_by_module_relative_path_to_workspace.find(module_relative_path_to_workspace);
    if (module_it != workspace->module_by_module_relative_path_to_workspace.end()) {
        return module_it->second;
    }
    const auto module_directory = absolute_path_to_workspace_directory / workspace_relative_path_to_workspace_ecosystem / module_relative_path_to_workspace;
    const auto module_version = derive_version(module_directory);
    auto module = new module_t {
        .workspace = workspace,
        .module_scc = nullptr,
        .module_builder = nullptr,
        .module_relative_path_to_workspace = module_relative_path_to_workspace,
        .version = module_version,
        .validated = false
    };
    auto builder = new builder_t {
        .produced_module = module,
        .builder_relative_path_to_produced_module = filesystem::relative_path_t(BUILDER_CPP),
        .version = module_version
    };
    module->module_builder = builder;

    workspace->module_by_module_relative_path_to_workspace.emplace(module_relative_path_to_workspace, module);

    json_module_t json_module;
    {
        const auto module_json_path = module_directory / filesystem::relative_path_t(MODULE_JSON);
        if (!filesystem::exists(module_json_path)) {
            throw std::runtime_error(std::format("kernel::cpp_builder::graph::discover_module_impl: file does not exist: '{}'", module_json_path.string()));
        }

        std::ifstream ifs(module_json_path.string());
        if (!ifs) {
            throw std::runtime_error(std::format("kernel::cpp_builder::graph::discover_module_impl: failed to open file '{}'", module_json_path.string()));
        }

        try {
            nlohmann::json json = nlohmann::json::parse(ifs);
            json_module = json.get<decltype(json_module)>();
        } catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error(std::format("kernel::cpp_builder::graph::discover_module_impl: failed to parse JSON file '{}': {}", module_json_path.string(), e.what()));
        } catch (const nlohmann::json::exception& e) {
            throw std::runtime_error(std::format("kernel::cpp_builder::graph::discover_module_impl: failed to get JSON module from file '{}': {}", module_json_path.string(), e.what()));
        }
    }

    for (const auto& module_dependency : json_module.module_dependencies) {
        module->dependencies.insert(discover_module_impl(filesystem::relative_path_t(module_dependency.workspace_relative_path), filesystem::relative_path_t(module_dependency.module_relative_path)));
    }

    for (const auto& builder_dependency : json_module.builder_dependencies) {
        builder->dependencies.insert(discover_module_impl(filesystem::relative_path_t(builder_dependency.workspace_relative_path), filesystem::relative_path_t(builder_dependency.module_relative_path)));
    }

    return module;
}

void workspace_ecosystem_t::strong_connect(module_t* module, uint32_t& index, std::stack<module_t*>& S, std::unordered_map<module_t*, module_info_t>& module_info_by_module) {
    const auto module_info_by_module_it = module_info_by_module.find(module);
    if (module_info_by_module_it == module_info_by_module.end()) {
        throw std::runtime_error("kernel::cpp_builder::graph::strong_connect: module not found in module_info_by_module");
    }

    auto& module_info = module_info_by_module_it->second;
    module_info.index = index;
    module_info.lowlink = index;
    ++index;
    S.push(module);
    module_info.on_stack = true;

    for (const auto& dependency : module->dependencies) {
        const auto dependency_module_info_by_module_it = module_info_by_module.find(dependency);
        if (dependency_module_info_by_module_it == module_info_by_module.end()) {
            throw std::runtime_error("kernel::cpp_builder::graph::strong_connect: dependency module not found in module_info_by_module");
        }
        const auto& dependency_module_info = dependency_module_info_by_module_it->second;
        if (dependency_module_info.index == -1) {
            strong_connect(dependency, index, S, module_info_by_module);
            module_info.lowlink = std::min(module_info.lowlink, dependency_module_info.lowlink);
        } else if (dependency_module_info.on_stack) {
            module_info.lowlink = std::min(module_info.lowlink, dependency_module_info.index);
        }
    }

    if (module_info.lowlink == module_info.index) {
        module_scc_t* module_scc = new module_scc_t;
        while (1) {
            const auto neighbor_module = S.top();
            S.pop();
            const auto neighbor_module_info_by_module_it = module_info_by_module.find(neighbor_module);
            if (neighbor_module_info_by_module_it == module_info_by_module.end()) {
                throw std::runtime_error("kernel::cpp_builder::graph::strong_connect: neighbor module not found in module_info_by_module");
            }
            auto& neighbor_module_info = neighbor_module_info_by_module_it->second;
            neighbor_module_info.on_stack = false;
            module_scc->modules.push_back(neighbor_module);
            neighbor_module->module_scc = module_scc;
            
            if (neighbor_module == module) {
                break ;
            }
        }
    }
}

version_t workspace_ecosystem_t::version_sccs(module_scc_t* scc, std::unordered_map<module_scc_t*, version_t>& visited, version_t minimum_version) {
    if (auto it = visited.find(scc); it != visited.end()) {
        return it->second;
    }

    version_t result = minimum_version;

    for (const auto& dependency : scc->dependencies) {
        result.value = std::max(result.value, version_sccs(dependency, visited, minimum_version).value);
    }

    for (const auto& module : scc->modules) {
        result.value = std::max(result.value, module->version.value);
    }

    for (auto& module : scc->modules) {
        module->version = result;
    }

    visited.emplace(scc, result);

    return result;
}

module_t* workspace_ecosystem_t::discover_module(filesystem::relative_path_t workspace_relative_path_to_workspace_ecosystem, filesystem::relative_path_t module_relative_path_to_workspace) {
    auto result = discover_module_impl(workspace_relative_path_to_workspace_ecosystem, module_relative_path_to_workspace);

    auto this_workspace_it = workspace_by_workspace_relative_path_to_workspace_ecosystem.find(filesystem::relative_path_t(THIS_WORKSPACE));
    if (this_workspace_it == workspace_by_workspace_relative_path_to_workspace_ecosystem.end()) {
        throw std::runtime_error(std::format("kernel::cpp_builder::graph::discover_module: this workspace '{}' not found in workspace ecosystem", THIS_WORKSPACE));
    }
    auto this_module_it = this_workspace_it->second->module_by_module_relative_path_to_workspace.find(filesystem::relative_path_t(THIS_MODULE));
    if (this_module_it == this_workspace_it->second->module_by_module_relative_path_to_workspace.end()) {
        throw std::runtime_error(std::format("kernel::cpp_builder::graph::discover_module: this module '{}' not found in workspace '{}'", THIS_MODULE, THIS_WORKSPACE));
    }
    this_workspace = this_workspace_it->second;
    this_module = this_module_it->second;

    std::unordered_map<module_t*, module_info_t> module_info_by_module;
    for (const auto& [_, workspace] : workspace_by_workspace_relative_path_to_workspace_ecosystem) {
        for (const auto& [_, module] : workspace->module_by_module_relative_path_to_workspace) {
            module_info_by_module[module] = module_info_t {
                .index = -1,
                .lowlink = -1,
                .on_stack = false
            };
        }
    }

    uint32_t index = 0;
    std::stack<module_t*> S;
    for (const auto& [module, module_info] : module_info_by_module) {
        if (module_info.index == -1) {
            strong_connect(module, index, S, module_info_by_module);
        }
    }

    std::unordered_map<module_scc_t*, std::unordered_set<module_scc_t*>> module_scc_dependencies_by_module_scc;
    for (const auto& [_, workspace] : workspace_by_workspace_relative_path_to_workspace_ecosystem) {
        for (const auto& [_, module] : workspace->module_by_module_relative_path_to_workspace) {
            for (const auto& dependency : module->dependencies) {
                assert(dependency->module_scc != nullptr);
                assert(module->module_scc != nullptr);

                if (dependency->module_scc != module->module_scc && module_scc_dependencies_by_module_scc[module->module_scc].insert(dependency->module_scc).second) {
                    module->module_scc->dependencies.push_back(dependency->module_scc);
                }
            }
        }
    }

    std::vector<module_t*> modules;
    for (const auto& [_, workspace] : workspace_by_workspace_relative_path_to_workspace_ecosystem) {
        for (const auto& [_, module] : workspace->module_by_module_relative_path_to_workspace) {
            modules.push_back(module);
        }
    }

    const auto propagate_module_dependency_versions = [&]() {
        std::unordered_map<module_scc_t*, version_t> visited;
        for (auto* module : modules) {
            version_sccs(module->module_scc, visited, this_module->version);
        }
    };

    for (std::size_t i = 0; i <= modules.size(); ++i) {
        propagate_module_dependency_versions();

        bool changed = false;
        for (auto* module : modules) {
            version_t builder_version = module->version;
            for (auto* builder_dependency : module->module_builder->dependencies) {
                builder_version.value = std::max(builder_version.value, builder_dependency->version.value);
            }

            if (module->module_builder->version.value < builder_version.value) {
                module->module_builder->version = builder_version;
                changed = true;
            }

            if (module->version.value < builder_version.value) {
                module->version = builder_version;
                changed = true;
            }
        }

        if (!changed) {
            break ;
        }
    }

    return result;
}

void module_t::validate() {
    if (validated) {
        return ;
    }
    validated = true;

    if (this == workspace->workspace_ecosystem->this_module) {
        // assume another producer exists instead of the builder to build this module
        return ;
    }

    const auto level = workspace->level;
    for (const auto& module_dependency : dependencies) {
        const auto module_dependency_level = module_dependency->workspace->level;
        if (!(module_dependency_level <= level)) {
            throw std::runtime_error(std::format("kernel::cpp_builder::graph::validate_module: module (workspace: {}, module: {}, level: {}) cannot depend on higher level module (workspace: {}, module: {}, level: {})", workspace->workspace_relative_path_to_workspace_ecosystem.string(), module_relative_path_to_workspace.string(), level, module_dependency->workspace->workspace_relative_path_to_workspace_ecosystem.string(), module_dependency->module_relative_path_to_workspace.string(), module_dependency_level));
        }

        module_dependency->validate();
    }

    const auto builder = module_builder;
    const auto builder_level = level;
    for (const auto builder_dependency : builder->dependencies) {
        const auto builder_dependency_level = builder_dependency->workspace->level;
        if (!(builder_dependency_level < builder_level)) {
            throw std::runtime_error(std::format("kernel::cpp_builder::graph::validate_module: builder (workspace: {}, module: {}, level: {}) cannot depend on same or higher level module (workspace: {}, module: {}, level: {})", workspace->workspace_relative_path_to_workspace_ecosystem.string(), module_relative_path_to_workspace.string(), builder_level, builder_dependency->workspace->workspace_relative_path_to_workspace_ecosystem.string(), builder_dependency->module_relative_path_to_workspace.string(), builder_dependency_level));
        }

        builder_dependency->validate();
    }
}

} // namespace graph

} // namespace cpp_builder

} // namespace kernel
