#include "module_builder.h"

#include "compiler.h"
#include <format>
#include <functional>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>

namespace kernel {

namespace cpp_builder {

namespace compiler {

filesystem::path_t create_builder_shared_library(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const filesystem::path_t& builder_source_file,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<filesystem::path_t>& libraries,
    const filesystem::path_t& builder_plugin
);

} // namespace compiler

} // namespace cpp_builder

} // namespace kernel

namespace kernel {

namespace cpp_builder {

namespace builder {

namespace {

std::string quote_define_value(std::string_view value) {
    std::string result = "\"";
    for (const char c : value) {
        if (c == '\\' || c == '"') {
            result.push_back('\\');
        }
        result.push_back(c);
    }
    result.push_back('"');
    return result;
}

std::vector<std::pair<std::string, std::string>> tool_path_defines() {
    return {
        { "KERNEL_CPP_BUILDER_CXX_COMPILER_PATH", quote_define_value(compiler::CXX_COMPILER_PATH) },
        { "KERNEL_CPP_BUILDER_CC_COMPILER_PATH", quote_define_value(compiler::CC_COMPILER_PATH) },
        { "KERNEL_CPP_BUILDER_AR_PATH", quote_define_value(compiler::AR_PATH) }
    };
}

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
            for (const auto& module_library_group : output.library_groups) {
                library_group.insert(library_group.end(), module_library_group.begin(), module_library_group.end());
            }
        }

        if (!library_group.empty()) {
            result.emplace_back(std::move(library_group));
        }
    });

    return result;
}

filesystem::path_t module_builder_t::builder_source_path() const {
    return builder_source_path(m_module);
}

filesystem::path_t module_builder_t::builder_dir() const {
    return builder_dir(m_module);
}

filesystem::path_t module_builder_t::builder_build_dir() const {
    return builder_build_dir(m_module);
}

filesystem::path_t module_builder_t::builder_install_dir() const {
    return builder_install_dir(m_module);
}

filesystem::path_t module_builder_t::builder_install_path() const {
    return builder_install_path(m_module);
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

void module_builder_t::publish_latest_stage(const iphase_t& phase) const {
    const auto latest_dir = artifact_latest_dir(phase.module());
    const auto latest_stage_dir = latest_dir / artifact_dir(phase.module()).relative(phase.artifact_dir());
    const auto latest_stage_tmp_dir = latest_stage_dir + "_tmp";

    if (filesystem::exists(latest_stage_tmp_dir)) {
        filesystem::remove_all(latest_stage_tmp_dir);
    }

    if (!filesystem::exists(latest_dir)) {
        filesystem::create_directories(latest_dir);
    }

    filesystem::create_directory_symlink(phase.artifact_dir(), latest_stage_tmp_dir);
    filesystem::rename_replace(latest_stage_tmp_dir, latest_stage_dir);
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

filesystem::path_t module_builder_t::build_builder(graph::module_t& module) const {
    const auto builder_plugin = builder_install_path(module);
    if (filesystem::exists(builder_plugin)) {
        return builder_plugin;
    }

    std::vector<filesystem::path_t> include_dirs;
    std::vector<filesystem::path_t> libraries;

    for (auto* dependency : module.module_builder->dependencies) {
        module_builder_t dependency_builder(m_workspace_ecosystem, *dependency);
        auto dependency_interfaces = dependency_builder.interface_roots(library_type_t::SHARED);
        include_dirs.insert(include_dirs.end(), std::make_move_iterator(dependency_interfaces.begin()), std::make_move_iterator(dependency_interfaces.end()));

        auto dependency_library_groups = dependency_builder.library_groups(library_type_t::SHARED);
        for (auto& dependency_library_group : dependency_library_groups) {
            libraries.insert(libraries.end(), std::make_move_iterator(dependency_library_group.begin()), std::make_move_iterator(dependency_library_group.end()));
        }
    }

    compiler::create_builder_shared_library(
        builder_build_dir(module),
        source_dir(module),
        include_dirs,
        builder_source_path(module),
        tool_path_defines(),
        libraries,
        builder_plugin
    );

    if (!filesystem::exists(builder_plugin)) {
        throw std::runtime_error(std::format("kernel::cpp_builder::builder::module_builder_t::build_builder: expected builder plugin '{}' to exist but it does not", builder_plugin));
    }

    return builder_plugin;
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
