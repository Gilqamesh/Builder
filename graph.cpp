#include "graph.h"

#include "external/json.hpp"

#include <algorithm>
#include <fstream>
#include <format>
#include <stack>
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

struct json_module_t {
    std::vector<std::string> module_dependencies;
    std::vector<std::string> builder_dependencies;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(json_module_t, module_dependencies, builder_dependencies)

struct module_info_t {
    int index;
    int lowlink;
    bool on_stack;
};

class module_scc_t {
public:
    void add_module(module_t& module);
    void add_dependency(module_scc_t& dependency);
    const std::vector<module_t*>& modules() const;
    const std::vector<module_scc_t*>& dependencies() const;

private:
    std::vector<module_t*> m_modules;
    std::vector<module_scc_t*> m_dependencies;
};

class workspace_graph_storage_t {
public:
    void clear_sccs();
    void scc(module_t& module, module_scc_t& scc);
    module_scc_t& scc(const module_t& module) const;

private:
    std::unordered_map<const module_t*, module_scc_t*> m_scc_by_module;
};

workspace_graph_t::workspace_graph_t(filesystem::path_t workspace_root, filesystem::path_t artifact_dir):
    m_root(std::move(workspace_root)),
    m_artifact_root(std::move(artifact_dir)),
    m_kernel_workspace(nullptr),
    m_kernel_module(nullptr),
    m_storage(new workspace_graph_storage_t)
{
}

const filesystem::path_t& workspace_graph_t::root() const {
    return m_root;
}

const filesystem::path_t& workspace_graph_t::artifact_root() const {
    return m_artifact_root;
}

workspace_t& workspace_graph_t::kernel_workspace() const {
    if (m_kernel_workspace == nullptr) {
        throw std::runtime_error("kernel::graph::workspace_graph_t::kernel_workspace: kernel workspace has not been discovered");
    }

    return *m_kernel_workspace;
}

module_t& workspace_graph_t::kernel_module() const {
    if (m_kernel_module == nullptr) {
        throw std::runtime_error("kernel::graph::workspace_graph_t::kernel_module: kernel module has not been discovered");
    }

    return *m_kernel_module;
}

workspace_t::workspace_t(workspace_graph_t& workspace_graph, filesystem::relative_path_t relative_path, uint32_t order_position):
    m_workspace_graph(&workspace_graph),
    m_relative_path(std::move(relative_path)),
    m_order_position(order_position)
{
}

workspace_graph_t& workspace_t::graph() const {
    return *m_workspace_graph;
}

const filesystem::relative_path_t& workspace_t::relative_path() const {
    return m_relative_path;
}

uint32_t workspace_t::order_position() const {
    return m_order_position;
}

module_t::module_t(workspace_t& workspace, filesystem::relative_path_t name, version_t version):
    m_workspace(&workspace),
    m_version(version),
    m_name(std::move(name))
{
}

workspace_t& module_t::workspace() const {
    return *m_workspace;
}

version_t module_t::version() const {
    return m_version;
}

void module_t::version(version_t version) {
    m_version = version;
}

void module_t::add_dependency(module_t& dependency) {
    m_dependencies.insert(&dependency);
}

void module_t::add_builder_dependency(module_t& dependency) {
    m_builder_dependencies.insert(&dependency);
}

void module_scc_t::add_module(module_t& module) {
    m_modules.push_back(&module);
}

void module_scc_t::add_dependency(module_scc_t& dependency) {
    m_dependencies.push_back(&dependency);
}

const std::vector<module_t*>& module_scc_t::modules() const {
    return m_modules;
}

const std::vector<module_scc_t*>& module_scc_t::dependencies() const {
    return m_dependencies;
}

void workspace_graph_storage_t::clear_sccs() {
    m_scc_by_module.clear();
}

void workspace_graph_storage_t::scc(module_t& module, module_scc_t& scc) {
    m_scc_by_module[&module] = &scc;
}

module_scc_t& workspace_graph_storage_t::scc(const module_t& module) const {
    const auto it = m_scc_by_module.find(&module);
    if (it == m_scc_by_module.end()) {
        throw std::runtime_error(std::format("kernel::graph::workspace_graph_storage_t::scc: module '{}' has no SCC", module.name()));
    }

    return *it->second;
}

std::vector<module_group_t> module_t::closure_groups() const {
    return workspace().graph().closure_groups(*this);
}

std::vector<module_group_t> workspace_graph_t::closure_groups(const module_t& module) const {
    std::unordered_set<const module_scc_t*> visited_sccs;
    std::vector<module_group_t> result;

    const auto collect_scc = [&]<class self_t>(self_t& self, const module_scc_t& current_scc) -> void {
        if (!visited_sccs.insert(&current_scc).second) {
            return ;
        }

        for (const auto* dependency : current_scc.dependencies()) {
            self(self, *dependency);
        }

        module_group_t group;
        group.reserve(current_scc.modules().size());
        for (auto* module : current_scc.modules()) {
            group.push_back(module);
        }
        result.push_back(std::move(group));
    };

    collect_scc(collect_scc, m_storage->scc(module));

    return result;
}

void workspace_graph_t::load_module_index() {
    if (!m_workspace_by_relative_path.empty()) {
        return ;
    }

    json_workspace_order_manifest_t json_workspace_order_manifest;
    const auto workspaces_json_file = root() / filesystem::relative_path_t(WORKSPACES_JSON);
    if (!filesystem::exists(workspaces_json_file)) {
        throw std::runtime_error(std::format("kernel::graph::workspace_graph_t::load_module_index: file does not exist: '{}'", workspaces_json_file));
    }

    std::ifstream ifs(workspaces_json_file.string());
    if (!ifs) {
        throw std::runtime_error(std::format("kernel::graph::workspace_graph_t::load_module_index: failed to open file '{}'", workspaces_json_file));
    }

    try {
        nlohmann::json json = nlohmann::json::parse(ifs);
        json_workspace_order_manifest = json.get<decltype(json_workspace_order_manifest)>();
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error(std::format("kernel::graph::workspace_graph_t::load_module_index: failed to parse JSON file '{}': {}", workspaces_json_file, e.what()));
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error(std::format("kernel::graph::workspace_graph_t::load_module_index: failed to get JSON workspace order manifest from file '{}': {}", workspaces_json_file, e.what()));
    }

    for (std::size_t i = 0; i < json_workspace_order_manifest.workspaces.size(); ++i) {
        const auto workspace_relative_path = filesystem::relative_path_t(json_workspace_order_manifest.workspaces[i]);
        if (m_workspace_by_relative_path.find(workspace_relative_path) != m_workspace_by_relative_path.end()) {
            throw std::runtime_error(std::format("kernel::graph::workspace_graph_t::load_module_index: duplicate workspace '{}' in '{}'", workspace_relative_path, workspaces_json_file));
        }

        auto workspace = new workspace_t(*this, workspace_relative_path, static_cast<uint32_t>(i));

        m_workspace_by_relative_path.emplace(workspace_relative_path, workspace);
    }

    for (const auto* workspace : workspaces()) {
        const auto workspace_dir = root() / workspace->relative_path();
        if (!filesystem::exists(workspace_dir)) {
            continue ;
        }

        for (const auto& module_dir : filesystem::find(workspace_dir, filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_none)) {
            const auto module_name = workspace_dir.relative(module_dir);
            const auto module_json_path = module_dir / filesystem::relative_path_t(MODULE_JSON);
            if (!filesystem::exists(module_json_path)) {
                continue ;
            }

            const auto [it, inserted] = m_workspace_by_module_name.emplace(module_name, workspace);
            if (!inserted) {
                throw std::runtime_error(std::format(
                    "kernel::graph::workspace_graph_t::load_module_index: duplicate module name '{}' found in workspaces '{}' and '{}'; module names are globally unique",
                    module_name,
                    it->second->relative_path(),
                    workspace->relative_path()
                ));
            }
        }
    }
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

filesystem::relative_path_t module_t::name() const {
    return m_name;
}

filesystem::path_t module_t::source_dir() const {
    return m_workspace->graph().root() / m_workspace->relative_path() / m_name;
}

filesystem::path_t module_t::artifact_base_dir() const {
    return m_workspace->graph().artifact_root() / m_name;
}

filesystem::path_t module_t::artifact_dir() const {
    const auto versioned_dir_name = std::format("{}@{}", m_name.to_native_path().filename().string(), version().value);
    return artifact_base_dir() / filesystem::relative_path_t(versioned_dir_name);
}

filesystem::path_t module_t::artifact_latest_dir() const {
    return artifact_base_dir() / filesystem::relative_path_t("latest");
}

module_t* workspace_t::find_module(const filesystem::relative_path_t& module_relative_path) const {
    auto it = m_module_by_relative_path.find(module_relative_path);
    if (it == m_module_by_relative_path.end()) {
        return nullptr;
    }

    return it->second;
}

void workspace_t::add_module(module_t* module) {
    m_module_by_relative_path.emplace(module->name(), module);
}

static bool module_less(const module_t* lhs, const module_t* rhs) {
    if (lhs->workspace().order_position() != rhs->workspace().order_position()) {
        return lhs->workspace().order_position() < rhs->workspace().order_position();
    }

    return lhs->name().string() < rhs->name().string();
}

std::vector<module_t*> workspace_t::modules() const {
    std::vector<module_t*> result;
    result.reserve(m_module_by_relative_path.size());

    for (const auto& [_, module] : m_module_by_relative_path) {
        result.push_back(module);
    }

    std::sort(result.begin(), result.end(), module_less);

    return result;
}

std::vector<module_t*> module_t::dependencies() {
    std::vector<module_t*> result;
    result.reserve(m_dependencies.size());

    for (auto* dependency : m_dependencies) {
        result.push_back(dependency);
    }

    std::sort(result.begin(), result.end(), module_less);

    return result;
}

std::vector<const module_t*> module_t::dependencies() const {
    std::vector<const module_t*> result;
    result.reserve(m_dependencies.size());

    for (const auto* dependency : m_dependencies) {
        result.push_back(dependency);
    }

    std::sort(result.begin(), result.end(), module_less);

    return result;
}

std::vector<module_t*> module_t::builder_dependencies() {
    std::vector<module_t*> result;
    result.reserve(m_builder_dependencies.size());

    for (auto* dependency : m_builder_dependencies) {
        result.push_back(dependency);
    }

    std::sort(result.begin(), result.end(), module_less);

    return result;
}

std::vector<const module_t*> module_t::builder_dependencies() const {
    std::vector<const module_t*> result;
    result.reserve(m_builder_dependencies.size());

    for (const auto* dependency : m_builder_dependencies) {
        result.push_back(dependency);
    }

    std::sort(result.begin(), result.end(), module_less);

    return result;
}

std::vector<const workspace_t*> workspace_graph_t::workspaces() const {
    std::vector<const workspace_t*> result;
    result.reserve(m_workspace_by_relative_path.size());

    for (const auto& [_, workspace] : m_workspace_by_relative_path) {
        result.push_back(workspace);
    }

    std::sort(result.begin(), result.end(), [](const auto* lhs, const auto* rhs) {
        if (lhs->order_position() != rhs->order_position()) {
            return lhs->order_position() < rhs->order_position();
        }

        return lhs->relative_path().string() < rhs->relative_path().string();
    });

    return result;
}

std::vector<const module_t*> workspace_graph_t::modules() const {
    std::vector<const module_t*> result;

    for (const auto* workspace : workspaces()) {
        for (const auto* module : workspace->modules()) {
            result.push_back(module);
        }
    }

    std::sort(result.begin(), result.end(), module_less);

    return result;
}

module_t* workspace_graph_t::discover_module_impl(filesystem::relative_path_t module_name) {
    const auto it = m_workspace_by_module_name.find(module_name);
    if (it == m_workspace_by_module_name.end()) {
        throw std::runtime_error(std::format(
            "kernel::graph::workspace_graph_t::discover_module_impl: module '{}' not found in workspace graph",
            module_name
        ));
    }

    const auto workspace_relative_path = it->second->relative_path();
    auto workspace_it = m_workspace_by_relative_path.find(workspace_relative_path);
    if (workspace_it == m_workspace_by_relative_path.end()) {
        throw std::runtime_error(std::format("kernel::graph::discover_module_impl: workspace '{}' is not listed in workspace order manifest '{}'", workspace_relative_path, WORKSPACES_JSON));
    }
    auto workspace = workspace_it->second;
    if (auto* discovered_module = workspace->find_module(module_name); discovered_module != nullptr) {
        return discovered_module;
    }

    const auto module_directory = root() / workspace_relative_path / module_name;
    const auto module_version = derive_version(module_directory);
    auto module = new module_t(*workspace, module_name, module_version);

    workspace->add_module(module);

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
        module->add_dependency(*discover_module_impl(filesystem::relative_path_t(module_dependency)));
    }

    for (const auto& builder_dependency : json_module.builder_dependencies) {
        module->add_builder_dependency(*discover_module_impl(filesystem::relative_path_t(builder_dependency)));
    }

    return module;
}

static void strong_connect(
    module_t* module,
    uint32_t& index,
    std::stack<module_t*>& S,
    std::unordered_map<module_t*, module_info_t>& module_info_by_module,
    workspace_graph_storage_t& graph_storage
) {
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

    for (auto* dependency : module->dependencies()) {
        const auto dependency_module_info_by_module_it = module_info_by_module.find(dependency);
        if (dependency_module_info_by_module_it == module_info_by_module.end()) {
            throw std::runtime_error("kernel::graph::strong_connect: dependency module not found in module_info_by_module");
        }
        const auto& dependency_module_info = dependency_module_info_by_module_it->second;
        if (dependency_module_info.index == -1) {
            strong_connect(dependency, index, S, module_info_by_module, graph_storage);
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
            module_scc->add_module(*neighbor_module);
            graph_storage.scc(*neighbor_module, *module_scc);
            
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

    for (const auto& dependency : scc->dependencies()) {
        result.value = std::max(result.value, version_sccs(dependency, visited, minimum_version).value);
    }

    for (const auto& module : scc->modules()) {
        result.value = std::max(result.value, module->version().value);
    }

    for (auto& module : scc->modules()) {
        module->version(result);
    }

    visited.emplace(scc, result);

    return result;
}

static void validate_module(
    const workspace_graph_t& workspace_graph,
    module_t* module,
    std::unordered_set<module_t*>& validated_modules
) {
    if (!validated_modules.insert(module).second) {
        return ;
    }

    if (module == &workspace_graph.kernel_module()) {
        // The active Builder kernel is allowed to bootstrap through an existing builder.
        return ;
    }

    const auto order_position = module->workspace().order_position();
    for (auto* module_dependency : module->dependencies()) {
        const auto module_dependency_order_position = module_dependency->workspace().order_position();
        if (!(module_dependency_order_position <= order_position)) {
            throw std::runtime_error(std::format("kernel::graph::validate_module: module (workspace: {}, module: {}, order position: {}) cannot depend on later workspace module (workspace: {}, module: {}, order position: {})", module->workspace().relative_path(), module->name(), order_position, module_dependency->workspace().relative_path(), module_dependency->name(), module_dependency_order_position));
        }

        validate_module(workspace_graph, module_dependency, validated_modules);
    }

    for (auto* builder_dependency : module->builder_dependencies()) {
        const auto builder_dependency_order_position = builder_dependency->workspace().order_position();
        if (!(builder_dependency_order_position < order_position)) {
            throw std::runtime_error(std::format("kernel::graph::validate_module: builder (workspace: {}, module: {}, order position: {}) cannot depend on same or later workspace module (workspace: {}, module: {}, order position: {})", module->workspace().relative_path(), module->name(), order_position, builder_dependency->workspace().relative_path(), builder_dependency->name(), builder_dependency_order_position));
        }

        validate_module(workspace_graph, builder_dependency, validated_modules);
    }
}

module_t* workspace_graph_t::discover_module(filesystem::relative_path_t module_name) {
    load_module_index();
    auto* result = discover_module_impl(module_name);

    auto kernel_workspace_it = m_workspace_by_relative_path.find(filesystem::relative_path_t(THIS_WORKSPACE));
    if (kernel_workspace_it == m_workspace_by_relative_path.end()) {
        throw std::runtime_error(std::format("kernel::graph::discover_module: this workspace '{}' not found in workspace graph", THIS_WORKSPACE));
    }
    auto* kernel_module = kernel_workspace_it->second->find_module(filesystem::relative_path_t(THIS_MODULE));
    if (kernel_module == nullptr) {
        throw std::runtime_error(std::format("kernel::graph::discover_module: this module '{}' not found in workspace '{}'", THIS_MODULE, THIS_WORKSPACE));
    }
    m_kernel_workspace = kernel_workspace_it->second;
    m_kernel_module = kernel_module;

    std::unordered_map<module_t*, module_info_t> module_info_by_module;
    for (const auto& [_, workspace] : m_workspace_by_relative_path) {
        for (auto* module : workspace->modules()) {
            module_info_by_module[module] = module_info_t {
                .index = -1,
                .lowlink = -1,
                .on_stack = false
            };
        }
    }

    uint32_t index = 0;
    std::stack<module_t*> S;
    m_storage->clear_sccs();
    for (const auto& [module, module_info] : module_info_by_module) {
        if (module_info.index == -1) {
            strong_connect(module, index, S, module_info_by_module, *m_storage);
        }
    }

    std::unordered_map<module_scc_t*, std::unordered_set<module_scc_t*>> module_scc_dependencies_by_module_scc;
    for (const auto& [_, workspace] : m_workspace_by_relative_path) {
        for (auto* module : workspace->modules()) {
            for (auto* dependency : module->dependencies()) {
                auto& dependency_scc = m_storage->scc(*dependency);
                auto& module_scc = m_storage->scc(*module);

                if (&dependency_scc != &module_scc && module_scc_dependencies_by_module_scc[&module_scc].insert(&dependency_scc).second) {
                    module_scc.add_dependency(dependency_scc);
                }
            }
        }
    }

    std::vector<module_t*> modules;
    for (const auto& [_, workspace] : m_workspace_by_relative_path) {
        for (auto* module : workspace->modules()) {
            modules.push_back(module);
        }
    }

    const auto propagate_module_dependency_versions = [&]() {
        std::unordered_map<module_scc_t*, version_t> visited;
        for (auto* module : modules) {
            version_sccs(&m_storage->scc(*module), visited, m_kernel_module->version());
        }
    };

    for (std::size_t i = 0; i <= modules.size(); ++i) {
        propagate_module_dependency_versions();

        bool changed = false;
        for (auto* module : modules) {
            version_t builder_version = module->version();
            for (auto* builder_dependency : module->builder_dependencies()) {
                builder_version.value = std::max(builder_version.value, builder_dependency->version().value);
            }

            if (module->version().value < builder_version.value) {
                module->version(builder_version);
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
