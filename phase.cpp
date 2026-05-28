#include "phase.h"

#include "compiler.h"
#include "module_builder.h"
#include "shared_library.h"

#include <format>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
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
            auto dependency_interfaces = dependency_builder.interface_roots(library_type_t::SHARED);
            include_dirs.insert(include_dirs.end(), std::make_move_iterator(dependency_interfaces.begin()), std::make_move_iterator(dependency_interfaces.end()));

            auto dependency_library_groups = dependency_builder.library_groups(library_type_t::SHARED);
            for (auto& dependency_library_group : dependency_library_groups) {
                libraries.insert(libraries.end(), std::make_move_iterator(dependency_library_group.begin()), std::make_move_iterator(dependency_library_group.end()));
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
const typename phase_t::output_t& phase_base_t::materialize() const {
    const auto* phase = dynamic_cast<const phase_t*>(this);
    if (phase == nullptr) {
        const auto* previous_phase = predecessor();
        if (previous_phase == nullptr) {
            throw std::runtime_error(std::format("phase_base_t::materialize: requested phase is not in the current phase '{}' or its configured predecessor chain", name()));
        }

        const auto* previous_phase_base = dynamic_cast<const phase_base_t*>(previous_phase);
        if (previous_phase_base == nullptr) {
            throw std::runtime_error(std::format("phase_base_t::materialize: predecessor of phase '{}' is not a phase_base_t", name()));
        }

        return previous_phase_base->materialize<phase_t>();
    }

    const auto build_dir = this->build_dir();
    const auto install_dir = this->install_dir();
    const auto marker_path = [&](std::string_view state) {
        return build_dir / filesystem::relative_path_t(std::format("{}.{}", name(), state));
    };
    const auto publish_latest_stage = [&]() {
        const auto phase_artifact_dir = this->artifact_dir();
        const auto latest_dir = m_module_builder.artifact_latest_dir(m_module);
        const auto latest_stage_dir = latest_dir / m_module_builder.artifact_dir(m_module).relative(phase_artifact_dir);
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
        publish_latest_stage();
        return phase->output();
    }

    if (filesystem::exists(started_marker)) {
        throw std::runtime_error(std::format("phase_base_t::materialize: re-entry detected for phase '{}'", name()));
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

        phase->execute();
        const auto& result = phase->output();

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

template const source_phase_t::output_t& phase_base_t::materialize<source_phase_t>() const;
template const interface_phase_t::output_t& phase_base_t::materialize<interface_phase_t>() const;
template const library_phase_t::output_t& phase_base_t::materialize<library_phase_t>() const;
template const binary_phase_t::output_t& phase_base_t::materialize<binary_phase_t>() const;

source_phase_t::source_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("source", module_builder, module, library_type, predecessor),
    m_output(install_dir())
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

const source_phase_t::output_t& source_phase_t::output() const {
    m_output.source_root = install_dir();
    return m_output;
}

interface_phase_t::interface_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    producer_phase_t("interface", module_builder, module, library_type, predecessor)
{
}

const interface_phase_t::output_t& interface_phase_t::output() const {
    m_output.interfaces = { install_dir() };
    return m_output;
}

library_phase_t::library_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    producer_phase_t("library", module_builder, module, library_type, predecessor)
{
}

const library_phase_t::output_t& library_phase_t::output() const {
    m_output.libraries = filesystem::find(*this, !filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_all);
    return m_output;
}

binary_phase_t::binary_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    producer_phase_t("binary", module_builder, module, library_type, predecessor),
    m_output(install_dir())
{
}

const binary_phase_t::output_t& binary_phase_t::output() const {
    m_output.binary_root = install_dir();
    m_output.cli = m_output.binary_root / filesystem::relative_path_t("cli");
    if (!filesystem::exists(m_output.cli)) {
        throw std::runtime_error(std::format("kernel::cpp_builder::builder::binary_phase_t::output: expected module cli '{}' to exist", m_output.cli));
    }
    return m_output;
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
