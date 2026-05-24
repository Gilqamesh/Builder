#include "module_builder.h"

#include "compiler.h"
#include "shared_library.h"

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

std::string module_display_name(const graph::module_t& module) {
    return std::format("{}/{}", module.workspace->workspace_relative_path_to_workspace_ecosystem.string(), module.module_relative_path_to_workspace.string());
}

const char* phase_symbol_name(module_builder_t::phase_t phase) {
    switch (phase) {
        case module_builder_t::phase_t::EXPORT_INTERFACE: return "module_builder__export_interface";
        case module_builder_t::phase_t::EXPORT_LIBRARIES: return "module_builder__export_libraries";
        case module_builder_t::phase_t::IMPORT_LIBRARIES: return "module_builder__import_libraries";
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::phase_symbol_name: unknown phase {}", static_cast<std::underlying_type_t<module_builder_t::phase_t>>(phase)));
    }
}

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

std::vector<std::pair<std::string, std::string>> compiler_path_defines() {
    return {
        { "KERNEL_CPP_BUILDER_CPP_COMPILER_PATH", quote_define_value(compiler::CPP_COMPILER_PATH) },
        { "KERNEL_CPP_BUILDER_C_COMPILER_PATH", quote_define_value(compiler::C_COMPILER_PATH) },
        { "KERNEL_CPP_BUILDER_AR_PATH", quote_define_value(compiler::AR_PATH) }
    };
}

std::vector<filesystem::relative_path_t> kernel_library_source_files() {
    return {
        filesystem::relative_path_t("filesystem.cpp"),
        filesystem::relative_path_t("process.cpp"),
        filesystem::relative_path_t("compiler.cpp"),
        filesystem::relative_path_t("shared_library.cpp"),
        filesystem::relative_path_t("graph.cpp"),
        filesystem::relative_path_t("module_builder.cpp")
    };
}

void install_artifact(const filesystem::path_t& install_dir, const filesystem::path_t& artifact, const filesystem::relative_path_t& relative_install_path) {
    const auto target_path = install_dir / relative_install_path;
    const auto target_path_parent = target_path.parent();
    if (!filesystem::exists(target_path_parent)) {
        filesystem::create_directories(target_path_parent);
    }

    filesystem::copy(artifact, target_path);
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

void validate_phase_module(const module_builder_t& module_builder, const graph::module_t& module, std::string_view phase_name) {
    if (&module_builder.module() != &module) {
        throw std::runtime_error(std::format("kernel::cpp_builder::builder::{}: module builder for '{}' cannot construct phase for '{}'", phase_name, module_display_name(module_builder.module()), module_display_name(module)));
    }
}

} // namespace

phase_base_t::phase_base_t(std::string_view name, module_builder_t& module_builder, graph::module_t& module, const iphase_t* predecessor):
    m_name(name),
    m_module_builder(module_builder),
    m_module(module),
    m_predecessor(predecessor)
{
    validate_phase_module(m_module_builder, m_module, m_name);
}

std::string_view phase_base_t::name() const {
    return m_name;
}

const iphase_t* phase_base_t::predecessor() const {
    return m_predecessor;
}

graph::module_t& phase_base_t::module() const {
    return m_module;
}

module_builder_t& phase_base_t::module_builder() const {
    return m_module_builder;
}

source_phase_t::source_phase_t(module_builder_t& module_builder, graph::module_t& module, const iphase_t* predecessor):
    phase_base_t("source", module_builder, module, predecessor),
    m_output(module_builder.source_phase_install_dir())
{
}

filesystem::path_t source_phase_t::dir() const {
    return module_builder().source_phase_dir();
}

filesystem::path_t source_phase_t::build_dir() const {
    return module_builder().source_phase_build_dir();
}

filesystem::path_t source_phase_t::install_dir() const {
    return module_builder().source_phase_install_dir();
}

void source_phase_t::execute() const {
    const auto module_source_dir = module_builder().source_dir(module());
    for (const auto& source_path : filesystem::find(module_source_dir, !filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_all)) {
        const auto target_path = install_dir() / module_source_dir.relative(source_path);
        const auto target_path_parent = target_path.parent();
        if (!filesystem::exists(target_path_parent)) {
            filesystem::create_directories(target_path_parent);
        }
        filesystem::copy(source_path, target_path);
    }
}

const source_phase_t::output_t& source_phase_t::output() const {
    m_output.source_root = install_dir();
    return m_output;
}

export_interface_phase_t::export_interface_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("export_interface", module_builder, module, predecessor),
    library_type(library_type)
{
}

filesystem::path_t export_interface_phase_t::dir() const {
    return module_builder().interface_dir();
}

filesystem::path_t export_interface_phase_t::build_dir() const {
    return module_builder().interface_build_dir(library_type);
}

filesystem::path_t export_interface_phase_t::install_dir() const {
    return module_builder().interface_install_dir(library_type);
}

void export_interface_phase_t::execute() const {
    (void) materialize<source_phase_t>();
    module_builder().dispatch_phase(*this);
}

const export_interface_phase_t::output_t& export_interface_phase_t::output() const {
    m_output.interfaces = { install_dir() };
    return m_output;
}

export_libraries_phase_t::export_libraries_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("export_libraries", module_builder, module, predecessor),
    library_type(library_type)
{
}

filesystem::path_t export_libraries_phase_t::dir() const {
    return module_builder().libraries_dir();
}

filesystem::path_t export_libraries_phase_t::build_dir() const {
    return module_builder().libraries_build_dir(library_type);
}

filesystem::path_t export_libraries_phase_t::install_dir() const {
    return module_builder().libraries_install_dir(library_type);
}

void export_libraries_phase_t::execute() const {
    (void) materialize<source_phase_t>();
    (void) materialize<export_interface_phase_t>();
    module_builder().dispatch_phase(*this);
}

const export_libraries_phase_t::output_t& export_libraries_phase_t::output() const {
    auto libraries = filesystem::find(*this, !filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_all);

    m_output.library_groups.clear();
    if (!libraries.empty()) {
        m_output.library_groups.emplace_back(std::move(libraries));
    }
    return m_output;
}

import_libraries_phase_t::import_libraries_phase_t(module_builder_t& module_builder, graph::module_t& module, const iphase_t* predecessor):
    phase_base_t("import_libraries", module_builder, module, predecessor)
{
}

filesystem::path_t import_libraries_phase_t::dir() const {
    return module_builder().import_dir();
}

filesystem::path_t import_libraries_phase_t::build_dir() const {
    return module_builder().import_build_dir();
}

filesystem::path_t import_libraries_phase_t::install_dir() const {
    return module_builder().import_install_dir();
}

void import_libraries_phase_t::execute() const {
    (void) materialize<source_phase_t>();
    (void) materialize<export_libraries_phase_t>();
    module_builder().dispatch_phase(*this);
}

const import_libraries_phase_t::output_t& import_libraries_phase_t::output() const {
    m_output = {};
    return m_output;
}

phase_chain_t::phase_chain_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type):
    source(module_builder, module),
    export_interface(module_builder, module, library_type, &source),
    export_libraries(module_builder, module, library_type, &export_interface),
    import_libraries(module_builder, module, &export_libraries)
{
}

void install_interface(const export_interface_phase_t& phase, const filesystem::path_t& interface, const filesystem::relative_path_t& relative_install_path) {
    install_artifact(phase.install_dir(), interface, relative_install_path);
}

void install_library(const export_libraries_phase_t& phase, const filesystem::path_t& library, const filesystem::relative_path_t& relative_install_path) {
    install_artifact(phase.install_dir(), library, relative_install_path);
}

void install_import(const import_libraries_phase_t& phase, const filesystem::path_t& artifact, const filesystem::relative_path_t& relative_install_path) {
    install_artifact(phase.install_dir(), artifact, relative_install_path);
}

module_builder_t::module_builder_t(graph::workspace_ecosystem_t& workspace_ecosystem, graph::module_t& module):
    m_workspace_ecosystem(workspace_ecosystem),
    m_module(module)
{
}

graph::module_t& module_builder_t::module() const {
    return m_module;
}

std::vector<filesystem::path_t> module_builder_t::export_interfaces(library_type_t library_type) const {
    std::vector<filesystem::path_t> exported_interfaces;

    visit_sccs_topo(m_module.module_scc, [&](const graph::module_scc_t* scc) {
        for (auto* module : scc->modules) {
            module_builder_t module_builder(m_workspace_ecosystem, *module);
            phase_chain_t phase_chain(module_builder, *module, library_type);
            const auto& output = phase_chain.export_interface.materialize<export_interface_phase_t>();
            exported_interfaces.insert(exported_interfaces.end(), output.interfaces.begin(), output.interfaces.end());
        }
    });

    return exported_interfaces;
}

std::vector<std::vector<filesystem::path_t>> module_builder_t::export_libraries(library_type_t library_type) const {
    std::vector<std::vector<filesystem::path_t>> library_groups;

    visit_sccs_topo(m_module.module_scc, [&](const graph::module_scc_t* scc) {
        std::vector<filesystem::path_t> library_group;

        for (auto* module : scc->modules) {
            module_builder_t module_builder(m_workspace_ecosystem, *module);
            phase_chain_t phase_chain(module_builder, *module, library_type);
            const auto& output = phase_chain.export_libraries.materialize<export_libraries_phase_t>();
            for (const auto& module_library_group : output.library_groups) {
                library_group.insert(library_group.end(), module_library_group.begin(), module_library_group.end());
            }
        }

        if (!library_group.empty()) {
            library_groups.emplace_back(std::move(library_group));
        }
    });

    return library_groups;
}

void module_builder_t::import_libraries() const {
    run_phase(m_module, phase_t::IMPORT_LIBRARIES, library_type_t::SHARED);
}

void module_builder_t::install_interface(const filesystem::path_t& interface, const filesystem::relative_path_t& relative_install_path, library_type_t library_type) const {
    install_artifact(interface_install_dir(library_type), interface, relative_install_path);
}

void module_builder_t::install_library(const filesystem::path_t& library, const filesystem::relative_path_t& relative_install_path, library_type_t library_type) const {
    install_artifact(libraries_install_dir(library_type), library, relative_install_path);
}

void module_builder_t::install_import(const filesystem::path_t& artifact, const filesystem::relative_path_t& relative_install_path) const {
    install_artifact(import_install_dir(), artifact, relative_install_path);
}

filesystem::path_t module_builder_t::workspace_ecosystem_dir() const {
    return m_workspace_ecosystem.absolute_path_to_workspace_directory;
}

filesystem::path_t module_builder_t::artifact_dir() const {
    return artifact_dir(m_module);
}

filesystem::path_t module_builder_t::artifact_latest_dir() const {
    return artifact_latest_dir(m_module);
}

filesystem::path_t module_builder_t::source_phase_dir() const {
    return source_phase_dir(m_module);
}

filesystem::path_t module_builder_t::source_phase_build_dir() const {
    return source_phase_build_dir(m_module);
}

filesystem::path_t module_builder_t::source_phase_install_dir() const {
    return source_phase_install_dir(m_module);
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

filesystem::path_t module_builder_t::interface_dir() const {
    return interface_dir(m_module);
}

filesystem::path_t module_builder_t::interface_build_dir(library_type_t library_type) const {
    return interface_build_dir(m_module, library_type);
}

filesystem::path_t module_builder_t::interface_install_dir(library_type_t library_type) const {
    return interface_install_dir(m_module, library_type);
}

filesystem::path_t module_builder_t::libraries_dir() const {
    return libraries_dir(m_module);
}

filesystem::path_t module_builder_t::libraries_build_dir(library_type_t library_type) const {
    return libraries_build_dir(m_module, library_type);
}

filesystem::path_t module_builder_t::libraries_install_dir(library_type_t library_type) const {
    return libraries_install_dir(m_module, library_type);
}

filesystem::path_t module_builder_t::import_dir() const {
    return import_dir(m_module);
}

filesystem::path_t module_builder_t::import_build_dir() const {
    return import_build_dir(m_module);
}

filesystem::path_t module_builder_t::import_install_dir() const {
    return import_install_dir(m_module);
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
    const auto latest_stage_dir = latest_dir / artifact_dir(phase.module()).relative(phase.dir());
    const auto latest_stage_tmp_dir = latest_stage_dir + "_tmp";

    if (filesystem::exists(latest_stage_tmp_dir)) {
        filesystem::remove_all(latest_stage_tmp_dir);
    }

    if (!filesystem::exists(latest_dir)) {
        filesystem::create_directories(latest_dir);
    }

    filesystem::create_directory_symlink(phase.dir(), latest_stage_tmp_dir);
    filesystem::rename_replace(latest_stage_tmp_dir, latest_stage_dir);
}

filesystem::path_t module_builder_t::source_phase_dir(const graph::module_t& module) const {
    return artifact_dir(module) / filesystem::relative_path_t("source");
}

filesystem::path_t module_builder_t::source_phase_build_dir(const graph::module_t& module) const {
    return source_phase_dir(module) / build_relative_dir();
}

filesystem::path_t module_builder_t::source_phase_install_dir(const graph::module_t& module) const {
    return source_phase_dir(module) / install_relative_dir();
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

filesystem::path_t module_builder_t::interface_dir(const graph::module_t& module) const {
    return artifact_dir(module) / filesystem::relative_path_t("interface");
}

filesystem::path_t module_builder_t::interface_build_dir(const graph::module_t& module, library_type_t library_type) const {
    return interface_dir(module) / library_type_relative_dir(library_type) / build_relative_dir();
}

filesystem::path_t module_builder_t::interface_install_dir(const graph::module_t& module, library_type_t library_type) const {
    return interface_dir(module) / library_type_relative_dir(library_type) / install_relative_dir();
}

filesystem::path_t module_builder_t::libraries_dir(const graph::module_t& module) const {
    return artifact_dir(module) / filesystem::relative_path_t("libraries");
}

filesystem::path_t module_builder_t::libraries_build_dir(const graph::module_t& module, library_type_t library_type) const {
    return libraries_dir(module) / library_type_relative_dir(library_type) / build_relative_dir();
}

filesystem::path_t module_builder_t::libraries_install_dir(const graph::module_t& module, library_type_t library_type) const {
    return libraries_dir(module) / library_type_relative_dir(library_type) / install_relative_dir();
}

filesystem::path_t module_builder_t::import_dir(const graph::module_t& module) const {
    return artifact_dir(module) / filesystem::relative_path_t("import");
}

filesystem::path_t module_builder_t::import_build_dir(const graph::module_t& module) const {
    return import_dir(module) / build_relative_dir();
}

filesystem::path_t module_builder_t::import_install_dir(const graph::module_t& module) const {
    return import_dir(module) / install_relative_dir();
}

void module_builder_t::run_phase(graph::module_t& module, phase_t phase, library_type_t library_type) const {
    module_builder_t module_builder(m_workspace_ecosystem, module);
    phase_chain_t phase_chain(module_builder, module, library_type);

    switch (phase) {
        case phase_t::EXPORT_INTERFACE: {
            (void) phase_chain.export_interface.materialize<export_interface_phase_t>();
            return ;
        }
        case phase_t::EXPORT_LIBRARIES: {
            (void) phase_chain.export_libraries.materialize<export_libraries_phase_t>();
            return ;
        }
        case phase_t::IMPORT_LIBRARIES: {
            (void) phase_chain.import_libraries.materialize<import_libraries_phase_t>();
            return ;
        }
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::module_builder_t::run_phase: unknown phase {}", static_cast<std::underlying_type_t<phase_t>>(phase)));
    }
}

void module_builder_t::dispatch_phase(const export_interface_phase_t& phase) const {
    if (&phase.module() == m_workspace_ecosystem.this_module) {
        run_kernel_phase(phase);
    } else {
        run_module_producer_phase(phase);
    }
}

void module_builder_t::dispatch_phase(const export_libraries_phase_t& phase) const {
    if (&phase.module() == m_workspace_ecosystem.this_module) {
        run_kernel_phase(phase);
    } else {
        run_module_producer_phase(phase);
    }
}

void module_builder_t::dispatch_phase(const import_libraries_phase_t& phase) const {
    if (&phase.module() == m_workspace_ecosystem.this_module) {
        run_kernel_phase(phase);
    } else {
        run_module_producer_phase(phase);
    }
}

void module_builder_t::run_module_producer_phase(const export_interface_phase_t& phase) const {
    const auto builder_plugin = build_builder(phase.module());
    shared_library::loader_t loader(builder_plugin, shared_library::lifetime_t::PROCESS, shared_library::symbol_resolution_t::LAZY, shared_library::symbol_visibility_t::LOCAL);
    using fn_t = void (*)(const export_interface_phase_t*);
    fn_t fn = loader.resolve(phase_symbol_name(phase_t::EXPORT_INTERFACE));
    fn(&phase);
}

void module_builder_t::run_module_producer_phase(const export_libraries_phase_t& phase) const {
    const auto builder_plugin = build_builder(phase.module());
    shared_library::loader_t loader(builder_plugin, shared_library::lifetime_t::PROCESS, shared_library::symbol_resolution_t::LAZY, shared_library::symbol_visibility_t::LOCAL);
    using fn_t = void (*)(const export_libraries_phase_t*);
    fn_t fn = loader.resolve(phase_symbol_name(phase_t::EXPORT_LIBRARIES));
    fn(&phase);
}

void module_builder_t::run_module_producer_phase(const import_libraries_phase_t& phase) const {
    const auto builder_plugin = build_builder(phase.module());
    shared_library::loader_t loader(builder_plugin, shared_library::lifetime_t::PROCESS, shared_library::symbol_resolution_t::LAZY, shared_library::symbol_visibility_t::LOCAL);
    using fn_t = void (*)(const import_libraries_phase_t*);
    fn_t fn = loader.resolve(phase_symbol_name(phase_t::IMPORT_LIBRARIES));
    fn(&phase);
}

void module_builder_t::run_kernel_phase(const export_interface_phase_t& phase) const {
    const auto& module_source_dir = phase.materialize<source_phase_t>().source_root;

    for (const auto& interface : filesystem::find(module_source_dir, filesystem::find_include_predicate_t::h_file || filesystem::find_include_predicate_t::hpp_file, filesystem::find_descend_predicate_t::descend_all)) {
        builder::install_interface(phase, interface, module_source_dir.relative(interface));
    }
}

void module_builder_t::run_kernel_phase(const export_libraries_phase_t& phase) const {
    const auto library_name = [&]() {
        switch (phase.library_type) {
            case library_type_t::STATIC: return filesystem::relative_path_t("libcpp_builder.a");
            case library_type_t::SHARED: return filesystem::relative_path_t("libcpp_builder.so");
            default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::module_builder_t::run_kernel_phase: unknown library_type {}", static_cast<std::underlying_type_t<library_type_t>>(phase.library_type)));
        }
    }();

    switch (phase.library_type) {
        case library_type_t::STATIC: {
            compiler::create_static_library(
                phase,
                kernel_library_source_files(),
                compiler_path_defines(),
                library_name
            );
        } break ;
        case library_type_t::SHARED: {
            compiler::create_shared_library(
                phase,
                kernel_library_source_files(),
                compiler_path_defines(),
                {},
                library_name
            );
        } break ;
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::module_builder_t::run_kernel_phase: unknown library_type {}", static_cast<std::underlying_type_t<library_type_t>>(phase.library_type)));
    }
}

void module_builder_t::run_kernel_phase(const import_libraries_phase_t& phase) const {
    const auto& library_output = phase.materialize<export_libraries_phase_t>();

    compiler::create_binary(
        phase,
        { filesystem::relative_path_t("cli.cpp") },
        compiler_path_defines(),
        library_output.library_groups,
        true,
        filesystem::relative_path_t("cli")
    );
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
        auto dependency_interfaces = dependency_builder.export_interfaces(library_type_t::SHARED);
        include_dirs.insert(include_dirs.end(), std::make_move_iterator(dependency_interfaces.begin()), std::make_move_iterator(dependency_interfaces.end()));

        auto dependency_library_groups = dependency_builder.export_libraries(library_type_t::SHARED);
        for (auto& dependency_library_group : dependency_library_groups) {
            libraries.insert(libraries.end(), std::make_move_iterator(dependency_library_group.begin()), std::make_move_iterator(dependency_library_group.end()));
        }
    }

    compiler::create_builder_shared_library(
        builder_build_dir(module),
        source_dir(module),
        include_dirs,
        builder_source_path(module),
        compiler_path_defines(),
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
