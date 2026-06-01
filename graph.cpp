#include "graph.h"

#include "external/json.hpp"

#include <fstream>
#include <format>
#include <stack>
#include <cassert>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace kernel {

namespace graph {

struct json_workspace_order_manifest_t {
    std::vector<std::string> workspaces;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(json_workspace_order_manifest_t, workspaces)

struct json_module_dependency_t {
    std::string workspace_relative_path;
    std::string module_relative_path;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(json_module_dependency_t, workspace_relative_path, module_relative_path)

struct json_module_t {
    std::vector<json_module_dependency_t> module_dependencies;
    std::vector<json_module_dependency_t> builder_dependencies;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(json_module_t, module_dependencies, builder_dependencies)

struct module_info_t {
    int index;
    int lowlink;
    bool on_stack;
};

static void load_workspace_order(workspace_ecosystem_t& workspace_ecosystem) {
    if (!workspace_ecosystem.workspace_by_relative_path.empty()) {
        return ;
    }

    json_workspace_order_manifest_t json_workspace_order_manifest;
    const auto workspaces_json_file = workspace_ecosystem.workspace_root / filesystem::relative_path_t(WORKSPACES_JSON);
    if (!filesystem::exists(workspaces_json_file)) {
        throw std::runtime_error(std::format("kernel::graph::load_workspace_order: file does not exist: '{}'", workspaces_json_file));
    }

    std::ifstream ifs(workspaces_json_file.string());
    if (!ifs) {
        throw std::runtime_error(std::format("kernel::graph::load_workspace_order: failed to open file '{}'", workspaces_json_file));
    }

    try {
        nlohmann::json json = nlohmann::json::parse(ifs);
        json_workspace_order_manifest = json.get<decltype(json_workspace_order_manifest)>();
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error(std::format("kernel::graph::load_workspace_order: failed to parse JSON file '{}': {}", workspaces_json_file, e.what()));
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error(std::format("kernel::graph::load_workspace_order: failed to get JSON workspace order manifest from file '{}': {}", workspaces_json_file, e.what()));
    }

    for (std::size_t i = 0; i < json_workspace_order_manifest.workspaces.size(); ++i) {
        const auto workspace_relative_path = filesystem::relative_path_t(json_workspace_order_manifest.workspaces[i]);
        if (workspace_ecosystem.workspace_by_relative_path.find(workspace_relative_path) != workspace_ecosystem.workspace_by_relative_path.end()) {
            throw std::runtime_error(std::format("kernel::graph::load_workspace_order: duplicate workspace '{}' in '{}'", workspace_relative_path, workspaces_json_file));
        }

        auto workspace = new workspace_t {
            .workspace_ecosystem = &workspace_ecosystem,
            .relative_path = workspace_relative_path,
            .order_position = static_cast<uint32_t>(i)
        };

        workspace_ecosystem.workspace_by_relative_path.emplace(workspace_relative_path, workspace);
    }
}

filesystem::path_t module_t::source_dir() const {
    return workspace->workspace_ecosystem->workspace_root / workspace->relative_path / module_relative_path_to_workspace;
}

filesystem::path_t module_t::artifact_base_dir() const {
    return workspace->workspace_ecosystem->artifact_dir / workspace->relative_path / module_relative_path_to_workspace;
}

filesystem::path_t module_t::artifact_dir() const {
    const auto versioned_dir_name = std::format("{}@{}", module_relative_path_to_workspace.to_native_path().filename().string(), version.value);
    return artifact_base_dir() / filesystem::relative_path_t(versioned_dir_name);
}

filesystem::path_t module_t::artifact_latest_dir() const {
    return artifact_base_dir() / filesystem::relative_path_t("latest");
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

static module_t* discover_module_impl(
    workspace_ecosystem_t& workspace_ecosystem,
    filesystem::relative_path_t workspace_relative_path,
    filesystem::relative_path_t module_relative_path_to_workspace
) {
    load_workspace_order(workspace_ecosystem);

    auto workspace_it = workspace_ecosystem.workspace_by_relative_path.find(workspace_relative_path);
    if (workspace_it == workspace_ecosystem.workspace_by_relative_path.end()) {
        throw std::runtime_error(std::format("kernel::graph::discover_module_impl: workspace '{}' is not listed in workspace order manifest '{}'", workspace_relative_path, WORKSPACES_JSON));
    }
    auto workspace = workspace_it->second;
    auto module_it = workspace->module_by_module_relative_path_to_workspace.find(module_relative_path_to_workspace);
    if (module_it != workspace->module_by_module_relative_path_to_workspace.end()) {
        return module_it->second;
    }
    const auto module_directory = workspace_ecosystem.workspace_root / workspace_relative_path / module_relative_path_to_workspace;
    const auto module_version = derive_version(module_directory);
    auto module = new module_t {
        .workspace = workspace,
        .module_scc = nullptr,
        .module_relative_path_to_workspace = module_relative_path_to_workspace,
        .version = module_version
    };

    workspace->module_by_module_relative_path_to_workspace.emplace(module_relative_path_to_workspace, module);

    json_module_t json_module;
    {
        const auto module_json_path = module_directory / filesystem::relative_path_t(MODULE_JSON);
        if (!filesystem::exists(module_json_path)) {
            throw std::runtime_error(std::format("kernel::graph::discover_module_impl: file does not exist: '{}'", module_json_path));
        }

        std::ifstream ifs(module_json_path.string());
        if (!ifs) {
            throw std::runtime_error(std::format("kernel::graph::discover_module_impl: failed to open file '{}'", module_json_path));
        }

        try {
            nlohmann::json json = nlohmann::json::parse(ifs);
            json_module = json.get<decltype(json_module)>();
        } catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error(std::format("kernel::graph::discover_module_impl: failed to parse JSON file '{}': {}", module_json_path, e.what()));
        } catch (const nlohmann::json::exception& e) {
            throw std::runtime_error(std::format("kernel::graph::discover_module_impl: failed to get JSON module from file '{}': {}", module_json_path, e.what()));
        }
    }

    for (const auto& module_dependency : json_module.module_dependencies) {
        module->dependencies.insert(discover_module_impl(workspace_ecosystem, filesystem::relative_path_t(module_dependency.workspace_relative_path), filesystem::relative_path_t(module_dependency.module_relative_path)));
    }

    for (const auto& builder_dependency : json_module.builder_dependencies) {
        module->builder_dependencies.insert(discover_module_impl(workspace_ecosystem, filesystem::relative_path_t(builder_dependency.workspace_relative_path), filesystem::relative_path_t(builder_dependency.module_relative_path)));
    }

    return module;
}

static void strong_connect(module_t* module, uint32_t& index, std::stack<module_t*>& S, std::unordered_map<module_t*, module_info_t>& module_info_by_module) {
    const auto module_info_by_module_it = module_info_by_module.find(module);
    if (module_info_by_module_it == module_info_by_module.end()) {
        throw std::runtime_error("kernel::graph::strong_connect: module not found in module_info_by_module");
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
            throw std::runtime_error("kernel::graph::strong_connect: dependency module not found in module_info_by_module");
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
                throw std::runtime_error("kernel::graph::strong_connect: neighbor module not found in module_info_by_module");
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

static version_t version_sccs(module_scc_t* scc, std::unordered_map<module_scc_t*, version_t>& visited, version_t minimum_version) {
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

static void validate_module(const workspace_ecosystem_t& workspace_ecosystem, module_t* module, std::unordered_set<module_t*>& validated_modules) {
    if (!validated_modules.insert(module).second) {
        return ;
    }

    if (module == workspace_ecosystem.this_module) {
        // The active Builder kernel is allowed to bootstrap through an existing builder.
        return ;
    }

    const auto order_position = module->workspace->order_position;
    for (const auto& module_dependency : module->dependencies) {
        const auto module_dependency_order_position = module_dependency->workspace->order_position;
        if (!(module_dependency_order_position <= order_position)) {
            throw std::runtime_error(std::format("kernel::graph::validate_module: module (workspace: {}, module: {}, order position: {}) cannot depend on later workspace module (workspace: {}, module: {}, order position: {})", module->workspace->relative_path, module->module_relative_path_to_workspace, order_position, module_dependency->workspace->relative_path, module_dependency->module_relative_path_to_workspace, module_dependency_order_position));
        }

        validate_module(workspace_ecosystem, module_dependency, validated_modules);
    }

    for (const auto builder_dependency : module->builder_dependencies) {
        const auto builder_dependency_order_position = builder_dependency->workspace->order_position;
        if (!(builder_dependency_order_position < order_position)) {
            throw std::runtime_error(std::format("kernel::graph::validate_module: builder (workspace: {}, module: {}, order position: {}) cannot depend on same or later workspace module (workspace: {}, module: {}, order position: {})", module->workspace->relative_path, module->module_relative_path_to_workspace, order_position, builder_dependency->workspace->relative_path, builder_dependency->module_relative_path_to_workspace, builder_dependency_order_position));
        }

        validate_module(workspace_ecosystem, builder_dependency, validated_modules);
    }
}

module_t* workspace_ecosystem_t::discover_module(filesystem::relative_path_t workspace_relative_path, filesystem::relative_path_t module_relative_path_to_workspace) {
    auto result = discover_module_impl(*this, workspace_relative_path, module_relative_path_to_workspace);

    auto this_workspace_it = workspace_by_relative_path.find(filesystem::relative_path_t(THIS_WORKSPACE));
    if (this_workspace_it == workspace_by_relative_path.end()) {
        throw std::runtime_error(std::format("kernel::graph::discover_module: this workspace '{}' not found in workspace ecosystem", THIS_WORKSPACE));
    }
    auto this_module_it = this_workspace_it->second->module_by_module_relative_path_to_workspace.find(filesystem::relative_path_t(THIS_MODULE));
    if (this_module_it == this_workspace_it->second->module_by_module_relative_path_to_workspace.end()) {
        throw std::runtime_error(std::format("kernel::graph::discover_module: this module '{}' not found in workspace '{}'", THIS_MODULE, THIS_WORKSPACE));
    }
    this_workspace = this_workspace_it->second;
    this_module = this_module_it->second;

    std::unordered_map<module_t*, module_info_t> module_info_by_module;
    for (const auto& [_, workspace] : workspace_by_relative_path) {
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
    for (const auto& [_, workspace] : workspace_by_relative_path) {
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
    for (const auto& [_, workspace] : workspace_by_relative_path) {
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
            for (auto* builder_dependency : module->builder_dependencies) {
                builder_version.value = std::max(builder_version.value, builder_dependency->version.value);
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

    std::unordered_set<module_t*> validated_modules;
    validate_module(*this, result, validated_modules);

    return result;
}

} // namespace graph

} // namespace kernel
