#include "module_builder.h"

#include <format>
#include <functional>
#include <type_traits>
#include <unordered_set>

namespace kernel {

namespace cpp_builder {

namespace builder {

namespace {

void visit_sccs_topo_impl(const graph::module_scc_t* scc, const std::function<void(const graph::module_scc_t*)>& f, std::unordered_set<const graph::module_scc_t*>& visited) {
    if (!visited.insert(scc).second) {
        return ;
    }

    for (const auto* dependency : scc->dependencies) {
        visit_sccs_topo_impl(dependency, f, visited);
    }

    f(scc);
}

void visit_sccs_topo(const graph::module_scc_t* scc, const std::function<void(const graph::module_scc_t*)>& f) {
    std::unordered_set<const graph::module_scc_t*> visited;
    visit_sccs_topo_impl(scc, f, visited);
}

} // namespace

module_builder_t::module_builder_t(graph::workspace_ecosystem_t& workspace_ecosystem, graph::module_t& module):
    m_workspace_ecosystem(workspace_ecosystem),
    m_module(module)
{
}

graph::module_t& module_builder_t::module() const {
    return m_module;
}

std::vector<filesystem::path_t> module_builder_t::interface_roots(library_type_t library_type) const {
    std::vector<filesystem::path_t> result;

    visit_sccs_topo(m_module.module_scc, [&](const graph::module_scc_t* scc) {
        for (auto* module : scc->modules) {
            module_builder_t module_builder(m_workspace_ecosystem, *module);
            phase_chain_t phase_chain(module_builder, *module, library_type);
            const auto& output = phase_chain.interface.materialize<interface_phase_t>();
            result.insert(result.end(), output.interfaces.begin(), output.interfaces.end());
        }
    });

    return result;
}

std::vector<std::vector<filesystem::path_t>> module_builder_t::library_groups(library_type_t library_type) const {
    std::vector<std::vector<filesystem::path_t>> result;

    visit_sccs_topo(m_module.module_scc, [&](const graph::module_scc_t* scc) {
        std::vector<filesystem::path_t> library_group;

        for (auto* module : scc->modules) {
            module_builder_t module_builder(m_workspace_ecosystem, *module);
            phase_chain_t phase_chain(module_builder, *module, library_type);
            const auto& output = phase_chain.library.materialize<library_phase_t>();
            library_group.insert(library_group.end(), output.libraries.begin(), output.libraries.end());
        }

        if (!library_group.empty()) {
            result.emplace_back(std::move(library_group));
        }
    });

    return result;
}

filesystem::path_t module_builder_t::source_dir(const graph::module_t& module) const {
    return module.source_dir();
}

filesystem::path_t module_builder_t::artifact_base_dir(const graph::module_t& module) const {
    return m_workspace_ecosystem.artifact_dir / module.workspace->workspace_relative_path_to_workspace_ecosystem / module.module_relative_path_to_workspace;
}

filesystem::path_t module_builder_t::artifact_dir(const graph::module_t& module) const {
    const auto versioned_dir_name = std::format("{}@{}", module.module_relative_path_to_workspace.to_native_path().filename().string(), module.version.value);
    return artifact_base_dir(module) / filesystem::relative_path_t(versioned_dir_name);
}

filesystem::path_t module_builder_t::artifact_latest_dir(const graph::module_t& module) const {
    return artifact_base_dir(module) / filesystem::relative_path_t("latest");
}

filesystem::path_t module_builder_t::builder_source_path(const graph::module_t& module) const {
    return module.builder_path();
}

filesystem::path_t module_builder_t::builder_dir(const graph::module_t& module) const {
    return artifact_dir(module) / filesystem::relative_path_t("builder");
}

filesystem::path_t module_builder_t::builder_build_dir(const graph::module_t& module) const {
    return builder_dir(module) / build_relative_dir();
}

filesystem::path_t module_builder_t::builder_install_dir(const graph::module_t& module) const {
    return builder_dir(module) / install_relative_dir();
}

filesystem::path_t module_builder_t::builder_install_path(const graph::module_t& module) const {
    return builder_install_dir(module) / filesystem::relative_path_t("builder.so");
}

filesystem::path_t module_builder_t::builder_install_latest_path(const graph::module_t& module) const {
    return artifact_latest_dir(module) / filesystem::relative_path_t("builder/install/builder.so");
}

filesystem::relative_path_t module_builder_t::build_relative_dir() const {
    return filesystem::relative_path_t("build");
}

filesystem::relative_path_t module_builder_t::install_relative_dir() const {
    return filesystem::relative_path_t("install");
}

filesystem::relative_path_t module_builder_t::library_type_relative_dir(library_type_t library_type) const {
    switch (library_type) {
        case library_type_t::STATIC: return filesystem::relative_path_t("static");
        case library_type_t::SHARED: return filesystem::relative_path_t("shared");
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::module_builder_t::library_type_relative_dir: unknown library_type {}", static_cast<std::underlying_type_t<library_type_t>>(library_type)));
    }
}

} // namespace builder

} // namespace cpp_builder

} // namespace kernel
