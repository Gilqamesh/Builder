#include "phase.h"

#include "compiler.h"
#include "module_builder.h"
#include "shared_library.h"

#include <format>
#include <stdexcept>
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

void validate_phase_module(const module_builder_t& module_builder, const graph::module_t& module, std::string_view phase_name) {
    if (&module_builder.module() != &module) {
        throw std::runtime_error(std::format("kernel::cpp_builder::builder::{}: module builder for '{}' cannot construct phase for '{}'", phase_name, module_display_name(module_builder.module()), module_display_name(module)));
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
        filesystem::relative_path_t("phase.cpp"),
        filesystem::relative_path_t("module_builder.cpp")
    };
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

bool phase_base_t::is_kernel_module() const {
    return &m_module == m_module_builder.m_workspace_ecosystem.this_module;
}

template <class phase_t>
void phase_base_t::dispatch_producer(const phase_t& phase) const {
    const auto builder_plugin = m_module_builder.build_builder(m_module);
    shared_library::loader_t loader(builder_plugin, shared_library::lifetime_t::PROCESS, shared_library::symbol_resolution_t::LAZY, shared_library::symbol_visibility_t::LOCAL);
    using fn_t = void (*)(const phase_t*);
    const auto producer_symbol_name = phase.producer_symbol_name();
    fn_t fn = loader.resolve(producer_symbol_name.c_str());
    fn(&phase);
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
    const auto started_marker = marker_path("started");
    const auto complete_marker = marker_path("complete");

    if (filesystem::exists(complete_marker)) {
        m_module_builder.publish_latest_stage(*this);
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

        m_module_builder.publish_latest_stage(*this);
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
    phase_base_t("interface", module_builder, module, library_type, predecessor)
{
}

void interface_phase_t::execute() const {
    if (is_kernel_module()) {
        const auto& module_source_dir = materialize<source_phase_t>().source_root;

        for (const auto& interface : filesystem::find(module_source_dir, filesystem::find_include_predicate_t::h_file || filesystem::find_include_predicate_t::hpp_file, filesystem::find_descend_predicate_t::descend_all)) {
            builder::install_interface(*this, interface, module_source_dir.relative(interface));
        }
        return ;
    }

    dispatch_producer(*this);
}

const interface_phase_t::output_t& interface_phase_t::output() const {
    m_output.interfaces = { install_dir() };
    return m_output;
}

library_phase_t::library_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("library", module_builder, module, library_type, predecessor)
{
}

void library_phase_t::execute() const {
    if (is_kernel_module()) {
        const auto library_name = [&]() {
            switch (library_type) {
                case library_type_t::STATIC: return filesystem::relative_path_t("libcpp_builder.a");
                case library_type_t::SHARED: return filesystem::relative_path_t("libcpp_builder.so");
                default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::library_phase_t::execute: unknown library_type {}", static_cast<std::underlying_type_t<library_type_t>>(library_type)));
            }
        }();

        switch (library_type) {
            case library_type_t::STATIC: {
                compiler::create_static_library(
                    *this,
                    kernel_library_source_files(),
                    compiler_path_defines(),
                    library_name
                );
            } break ;
            case library_type_t::SHARED: {
                compiler::create_shared_library(
                    *this,
                    kernel_library_source_files(),
                    compiler_path_defines(),
                    {},
                    library_name
                );
            } break ;
            default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::library_phase_t::execute: unknown library_type {}", static_cast<std::underlying_type_t<library_type_t>>(library_type)));
        }
        return ;
    }

    dispatch_producer(*this);
}

const library_phase_t::output_t& library_phase_t::output() const {
    auto libraries = filesystem::find(*this, !filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_all);

    m_output.library_groups.clear();
    if (!libraries.empty()) {
        m_output.library_groups.emplace_back(std::move(libraries));
    }
    return m_output;
}

binary_phase_t::binary_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("binary", module_builder, module, library_type, predecessor),
    m_output(install_dir())
{
}

void binary_phase_t::execute() const {
    const auto& library_output = materialize<library_phase_t>();

    if (is_kernel_module()) {
        compiler::create_binary(
            *this,
            { filesystem::relative_path_t("cli.cpp") },
            compiler_path_defines(),
            library_output.library_groups,
            true,
            filesystem::relative_path_t("cli")
        );
        return ;
    }

    dispatch_producer(*this);
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
