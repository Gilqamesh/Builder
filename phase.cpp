#include "phase.h"

#include "compiler.h"
#include "module_builder.h"
#include "shared_library.h"

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace kernel {

namespace cpp_builder {

namespace builder {

namespace {

std::string module_display_name(const graph::module_t& module) {
    return std::format("{}/{}", module.workspace->workspace_relative_path_to_workspace_ecosystem.string(), module.module_relative_path_to_workspace.string());
}

void install_artifact(const filesystem::path_t& install_dir, const filesystem::path_t& artifact, const filesystem::relative_path_t& relative_install_path) {
    const auto target_path = install_dir / relative_install_path;
    const auto target_path_parent = target_path.parent();
    if (!filesystem::exists(target_path_parent)) {
        filesystem::create_directories(target_path_parent);
    }

    filesystem::copy(artifact, target_path);
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

std::vector<std::pair<std::string, std::string>> tool_path_defines() {
    return {
        { "KERNEL_CPP_BUILDER_CXX_COMPILER_PATH", quote_define_value(compiler::CXX_COMPILER_PATH) },
        { "KERNEL_CPP_BUILDER_CC_COMPILER_PATH", quote_define_value(compiler::CC_COMPILER_PATH) },
        { "KERNEL_CPP_BUILDER_AR_PATH", quote_define_value(compiler::AR_PATH) }
    };
}

void validate_phase_module(const module_builder_t& module_builder, const graph::module_t& module, std::string_view phase_name) {
    if (&module_builder.module() != &module) {
        throw std::runtime_error(std::format("kernel::cpp_builder::builder::{}: module builder for '{}' cannot construct phase for '{}'", phase_name, module_display_name(module_builder.module()), module_display_name(module)));
    }
}

template <class>
inline constexpr bool always_false_v = false;

} // namespace

source_output_t::source_output_t(const filesystem::path_t& configured_source_root):
    source_root(configured_source_root)
{
}

binary_output_t::binary_output_t(const filesystem::path_t& configured_binary_root):
    binary_root(configured_binary_root),
    cli(configured_binary_root / filesystem::relative_path_t("cli"))
{
}

source_output_t materialized_output(const source_phase_t& phase) {
    return source_output_t(phase.install_dir());
}

interface_output_t materialized_output(const interface_phase_t& phase) {
    return interface_output_t {
        .interfaces = { phase.install_dir() }
    };
}

library_output_t materialized_output(const library_phase_t& phase) {
    return library_output_t {
        .libraries = filesystem::find(phase, !filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_all)
    };
}

binary_output_t materialized_output(const binary_phase_t& phase) {
    binary_output_t output(phase.install_dir());
    if (!filesystem::exists(output.cli)) {
        throw std::runtime_error(std::format("kernel::cpp_builder::builder::binary_phase_t::materialized_output: expected module cli '{}' to exist", output.cli));
    }
    return output;
}

template <class phase_t>
const phase_t& phase_from_chain(const phase_chain_t& phase_chain) {
    if constexpr (std::is_same_v<phase_t, source_phase_t>) {
        return phase_chain.source;
    } else if constexpr (std::is_same_v<phase_t, interface_phase_t>) {
        return phase_chain.interface;
    } else if constexpr (std::is_same_v<phase_t, library_phase_t>) {
        return phase_chain.library;
    } else if constexpr (std::is_same_v<phase_t, binary_phase_t>) {
        return phase_chain.binary;
    } else {
        static_assert(always_false_v<phase_t>, "unsupported phase type");
    }
}

phase_base_t::phase_base_t(std::string_view name, module_builder_t& module_builder, graph::module_t& module, library_type_t configured_library_type, const iphase_t* predecessor):
    library_type(configured_library_type),
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

filesystem::path_t phase_base_t::artifact_dir() const {
    return m_module_builder.artifact_dir(m_module) / filesystem::relative_path_t(std::string(m_name));
}

filesystem::path_t phase_base_t::build_dir() const {
    return artifact_dir() / m_module_builder.build_relative_dir() / m_module_builder.library_type_relative_dir(library_type);
}

filesystem::path_t phase_base_t::install_dir() const {
    return artifact_dir() / m_module_builder.install_relative_dir() / m_module_builder.library_type_relative_dir(library_type);
}

std::string phase_base_t::producer_symbol_name() const {
    return std::format("phase__{}", m_name);
}

module_builder_t& phase_base_t::module_builder() const {
    return m_module_builder;
}

template <class phase_t>
void producer_phase_t<phase_t>::execute() const {
    auto& module_builder = this->module_builder();
    auto& module = this->module();
    const auto builder_plugin = [&]() {
        if (&module == module_builder.m_workspace_ecosystem.this_module) {
            const auto latest_builder_plugin = module_builder.builder_install_latest_path(module);
            if (filesystem::exists(latest_builder_plugin)) {
                return latest_builder_plugin;
            }
        }

        const auto builder_plugin = module_builder.builder_install_path(module);
        if (filesystem::exists(builder_plugin)) {
            return builder_plugin;
        }

        std::vector<filesystem::path_t> include_dirs;
        std::vector<filesystem::path_t> libraries;

        for (auto* dependency : module.module_builder->dependencies) {
            module_builder_t dependency_builder(module_builder.m_workspace_ecosystem, *dependency);
            phase_chain_t dependency_phase_chain(dependency_builder, *dependency, library_type_t::SHARED);

            const auto dependency_interface_outputs = dependency_phase_chain.interface.materialize<interface_phase_t>();
            for (const auto& dependency_interface_output : dependency_interface_outputs) {
                include_dirs.insert(include_dirs.end(), dependency_interface_output.interfaces.begin(), dependency_interface_output.interfaces.end());
            }

            const auto dependency_library_outputs = dependency_phase_chain.library.materialize<library_phase_t>();
            for (const auto& dependency_library_output : dependency_library_outputs) {
                libraries.insert(libraries.end(), dependency_library_output.libraries.begin(), dependency_library_output.libraries.end());
            }
        }

        compiler::create_builder_shared_library(
            module_builder.builder_build_dir(module),
            module_builder.source_dir(module),
            include_dirs,
            module_builder.builder_source_path(module),
            tool_path_defines(),
            libraries,
            builder_plugin
        );

        if (!filesystem::exists(builder_plugin)) {
            throw std::runtime_error(std::format("kernel::cpp_builder::builder::producer_phase_t::execute: expected builder plugin '{}' to exist", builder_plugin));
        }

        return builder_plugin;
    }();

    shared_library::loader_t loader(builder_plugin, shared_library::lifetime_t::PROCESS, shared_library::symbol_resolution_t::LAZY, shared_library::symbol_visibility_t::LOCAL);
    using fn_t = void (*)(const phase_t*);
    const auto producer_symbol_name = this->producer_symbol_name();
    fn_t fn = loader.resolve(producer_symbol_name.c_str());
    fn(static_cast<const phase_t*>(this));
}

template <class phase_t>
const phase_t& phase_base_t::exact_phase() const {
    const auto* phase = dynamic_cast<const phase_t*>(this);
    if (phase == nullptr) {
        const auto* previous_phase = predecessor();
        if (previous_phase == nullptr) {
            throw std::runtime_error(std::format("phase_base_t::exact_phase: requested phase is not in the current phase '{}' or its configured predecessor chain", name()));
        }

        const auto* previous_phase_base = dynamic_cast<const phase_base_t*>(previous_phase);
        if (previous_phase_base == nullptr) {
            throw std::runtime_error(std::format("phase_base_t::exact_phase: predecessor of phase '{}' is not a phase_base_t", name()));
        }

        return previous_phase_base->exact_phase<phase_t>();
    }

    return *phase;
}

template <class phase_t>
typename phase_t::output_t phase_base_t::materialize_exact() const {
    const auto& phase = exact_phase<phase_t>();
    const auto build_dir = phase.build_dir();
    const auto install_dir = phase.install_dir();
    const auto marker_path = [&](std::string_view state) {
        return build_dir / filesystem::relative_path_t(std::format("{}.{}", phase.name(), state));
    };
    const auto publish_latest_stage = [&]() {
        const auto phase_artifact_dir = phase.artifact_dir();
        const auto latest_dir = phase.m_module_builder.artifact_latest_dir(phase.m_module);
        const auto latest_stage_dir = latest_dir / phase.m_module_builder.artifact_dir(phase.m_module).relative(phase_artifact_dir);
        const auto latest_stage_tmp_dir = latest_stage_dir + "_tmp";

        if (filesystem::exists(latest_stage_tmp_dir)) {
            filesystem::remove_all(latest_stage_tmp_dir);
        }

        if (!filesystem::exists(latest_dir)) {
            filesystem::create_directories(latest_dir);
        }

        filesystem::create_directory_symlink(phase_artifact_dir, latest_stage_tmp_dir);
        filesystem::rename_replace(latest_stage_tmp_dir, latest_stage_dir);
    };
    const auto started_marker = marker_path("started");
    const auto complete_marker = marker_path("complete");

    if (filesystem::exists(complete_marker)) {
        auto result = materialized_output(phase);
        publish_latest_stage();
        return result;
    }

    if (filesystem::exists(started_marker)) {
        throw std::runtime_error(std::format("phase_base_t::materialize_exact: re-entry detected for phase '{}'", phase.name()));
    }

    if (filesystem::exists(build_dir)) {
        filesystem::remove_all(build_dir);
    }
    if (filesystem::exists(install_dir)) {
        filesystem::remove_all(install_dir);
    }

    try {
        if (!filesystem::exists(build_dir)) {
            filesystem::create_directories(build_dir);
        }
        filesystem::touch(started_marker);
        filesystem::create_directories(install_dir);

        phase.execute();
        auto result = materialized_output(phase);

        publish_latest_stage();
        filesystem::touch(complete_marker);
        filesystem::remove(started_marker);

        return result;
    } catch (...) {
        filesystem::remove_all(build_dir);
        filesystem::remove_all(install_dir);
        throw ;
    }
}

template <class phase_t>
std::vector<typename phase_t::output_t> phase_base_t::materialize() const {
    std::unordered_set<const graph::module_scc_t*> visited_sccs;
    return materialize<phase_t>(visited_sccs);
}

template <class phase_t>
std::vector<typename phase_t::output_t> phase_base_t::materialize(std::unordered_set<const graph::module_scc_t*>& visited_sccs) const {
    const auto* scc = module().module_scc;
    if (scc == nullptr) {
        throw std::runtime_error(std::format("phase_base_t::materialize: module '{}' has no SCC", module_display_name(module())));
    }

    std::vector<typename phase_t::output_t> outputs;
    append_materialized_scc_outputs<phase_t>(*scc, visited_sccs, outputs);
    return outputs;
}

template <class phase_t>
void phase_base_t::append_materialized_scc_outputs(const graph::module_scc_t& scc, std::unordered_set<const graph::module_scc_t*>& visited_sccs, std::vector<typename phase_t::output_t>& outputs) const {
    if (!visited_sccs.insert(&scc).second) {
        return ;
    }

    for (const auto* dependency : scc.dependencies) {
        append_materialized_scc_outputs<phase_t>(*dependency, visited_sccs, outputs);
    }

    for (auto* module : scc.modules) {
        module_builder_t module_builder(m_module_builder.m_workspace_ecosystem, *module);
        phase_chain_t phase_chain(module_builder, *module, library_type);
        outputs.push_back(phase_from_chain<phase_t>(phase_chain).template materialize_exact<phase_t>());
    }
}

template std::vector<source_phase_t::output_t> phase_base_t::materialize<source_phase_t>() const;
template std::vector<interface_phase_t::output_t> phase_base_t::materialize<interface_phase_t>() const;
template std::vector<library_phase_t::output_t> phase_base_t::materialize<library_phase_t>() const;
template std::vector<binary_phase_t::output_t> phase_base_t::materialize<binary_phase_t>() const;

template std::vector<source_phase_t::output_t> phase_base_t::materialize<source_phase_t>(std::unordered_set<const graph::module_scc_t*>&) const;
template std::vector<interface_phase_t::output_t> phase_base_t::materialize<interface_phase_t>(std::unordered_set<const graph::module_scc_t*>&) const;
template std::vector<library_phase_t::output_t> phase_base_t::materialize<library_phase_t>(std::unordered_set<const graph::module_scc_t*>&) const;
template std::vector<binary_phase_t::output_t> phase_base_t::materialize<binary_phase_t>(std::unordered_set<const graph::module_scc_t*>&) const;

source_phase_t::source_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("source", module_builder, module, library_type, predecessor)
{
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

interface_phase_t::interface_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    producer_phase_t("interface", module_builder, module, library_type, predecessor)
{
}

library_phase_t::library_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    producer_phase_t("library", module_builder, module, library_type, predecessor)
{
}

binary_phase_t::binary_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    producer_phase_t("binary", module_builder, module, library_type, predecessor)
{
}

phase_chain_t::phase_chain_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type):
    source(module_builder, module, library_type),
    interface(module_builder, module, library_type, &source),
    library(module_builder, module, library_type, &interface),
    binary(module_builder, module, library_type, &library)
{
}

void install_interface(const interface_phase_t& phase, const filesystem::path_t& interface, const filesystem::relative_path_t& relative_install_path) {
    install_artifact(phase.install_dir(), interface, relative_install_path);
}

void install_library(const library_phase_t& phase, const filesystem::path_t& library, const filesystem::relative_path_t& relative_install_path) {
    install_artifact(phase.install_dir(), library, relative_install_path);
}

void install_binary(const binary_phase_t& phase, const filesystem::path_t& artifact, const filesystem::relative_path_t& relative_install_path) {
    install_artifact(phase.install_dir(), artifact, relative_install_path);
}

} // namespace builder

} // namespace cpp_builder

} // namespace kernel
