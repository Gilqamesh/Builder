#include "phase.h"

#include "compiler.h"

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace kernel {

namespace cpp_builder {

namespace builder {

static std::string quote_define_value(std::string_view value) {
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

static std::vector<std::pair<std::string, std::string>> tool_path_defines() {
    return {
        { "KERNEL_CPP_BUILDER_CXX_COMPILER_PATH", quote_define_value(KERNEL_CPP_BUILDER_CXX_COMPILER_PATH) },
        { "KERNEL_CPP_BUILDER_CC_COMPILER_PATH", quote_define_value(KERNEL_CPP_BUILDER_CC_COMPILER_PATH) },
        { "KERNEL_CPP_BUILDER_AR_PATH", quote_define_value(KERNEL_CPP_BUILDER_AR_PATH) }
    };
}

static filesystem::relative_path_t library_type_relative_dir(library_type_t library_type) {
    switch (library_type) {
        case library_type_t::STATIC: return filesystem::relative_path_t("static");
        case library_type_t::SHARED: return filesystem::relative_path_t("shared");
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::library_type_relative_dir: unknown library_type {}", static_cast<std::underlying_type_t<library_type_t>>(library_type)));
    }
}

phase_base_t::phase_base_t(std::string_view name, graph::module_t& module, library_type_t configured_library_type, const iphase_t* predecessor):
    m_name(name),
    m_module(module),
    m_library_type(configured_library_type),
    m_predecessor(predecessor)
{
}

std::string_view phase_base_t::name() const {
    return m_name;
}

const iphase_t* phase_base_t::predecessor() const {
    return m_predecessor;
}

filesystem::path_t phase_base_t::artifact_dir() const {
    return m_module.artifact_dir() / filesystem::relative_path_t(std::string(m_name));
}

filesystem::path_t phase_base_t::build_dir() const {
    return artifact_dir() / filesystem::relative_path_t("build") / library_type_relative_dir(library_type());
}

filesystem::path_t phase_base_t::install_dir() const {
    return artifact_dir() / filesystem::relative_path_t("install") / library_type_relative_dir(library_type());
}

library_type_t phase_base_t::library_type() const {
    return m_library_type;
}

filesystem::path_t phase_base_t::source_install_dir() const {
    const phase_base_t* current_phase = this;

    while (current_phase != nullptr) {
        if (const auto* source_phase = dynamic_cast<const source_phase_t*>(current_phase)) {
            return source_phase->install_dir();
        }

        const auto* previous_phase = current_phase->predecessor();
        if (previous_phase == nullptr) {
            break ;
        }

        current_phase = dynamic_cast<const phase_base_t*>(previous_phase);
        if (current_phase == nullptr) {
            throw std::runtime_error(std::format("kernel::cpp_builder::builder::phase_base_t::source_install_dir: predecessor of phase '{}' is not a phase_base_t", name()));
        }
    }

    throw std::runtime_error(std::format("kernel::cpp_builder::builder::phase_base_t::source_install_dir: phase '{}' has no configured source phase", name()));
}

void phase_base_t::declare_output(const filesystem::path_t& artifact, const filesystem::relative_path_t& relative_install_path) const {
    m_declared_outputs.push_back(declared_output_t {
        .artifact = artifact,
        .relative_install_path = relative_install_path
    });
}

std::string phase_base_t::producer_symbol_name() const {
    return std::format("phase__{}", m_name);
}

filesystem::path_t phase_base_t::builder_plugin() const {
    auto& module = m_module;
    if (&module == module.workspace->workspace_ecosystem->this_module) {
        const auto latest_builder_plugin = module.builder_install_latest_path();
        if (filesystem::exists(latest_builder_plugin)) {
            return latest_builder_plugin;
        }
    }

    const auto builder_plugin = module.builder_install_path();
    if (filesystem::exists(builder_plugin)) {
        return builder_plugin;
    }

    std::vector<filesystem::path_t> include_dirs;
    std::vector<filesystem::path_t> libraries;

    for (auto* dependency : module.module_builder->dependencies) {
        dependency->configure(library_type_t::SHARED);

        for (const auto& dependency_include_dirs : dependency->materialize_all<interface_phase_t>()) {
            include_dirs.insert(include_dirs.end(), dependency_include_dirs.include_dirs.begin(), dependency_include_dirs.include_dirs.end());
        }

        for (const auto& dependency_libraries : dependency->materialize_all<library_phase_t>()) {
            libraries.insert(libraries.end(), dependency_libraries.libraries.begin(), dependency_libraries.libraries.end());
        }
    }

    compiler::create_builder_shared_library(
        module.builder_build_dir(),
        module.source_dir(),
        include_dirs,
        module.builder_path(),
        tool_path_defines(),
        libraries,
        builder_plugin
    );

    if (!filesystem::exists(builder_plugin)) {
        throw std::runtime_error(std::format("kernel::cpp_builder::builder::phase_base_t::builder_plugin: expected builder plugin '{}' to exist", builder_plugin));
    }

    return builder_plugin;
}

source_phase_t::source_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("source", module, library_type, predecessor)
{
}

filesystem::path_t source_phase_t::source_dir() const {
    return m_module.source_dir();
}

void source_phase_t::add_source(const filesystem::path_t& source, const filesystem::relative_path_t& relative_install_path) const {
    declare_output(source, relative_install_path);
}

source_phase_t::output_t source_phase_t::installed_output() const {
    return output_t {
        .sources = filesystem::find(install_dir(), !filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_all)
    };
}

interface_phase_t::interface_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("interface", module, library_type, predecessor)
{
}

filesystem::relative_path_t interface_phase_t::source_relative_path(const filesystem::path_t& source) const {
    const auto* source_phase = dynamic_cast<const source_phase_t*>(predecessor());
    if (source_phase == nullptr) {
        throw std::runtime_error("kernel::cpp_builder::builder::interface_phase_t::source_relative_path: interface phase predecessor is not a source phase");
    }

    return source_phase->install_dir().relative(source);
}

void interface_phase_t::add_interface(const filesystem::path_t& interface) const {
    add_interface(interface, source_relative_path(interface));
}

void interface_phase_t::add_interface(const filesystem::path_t& interface, const filesystem::relative_path_t& relative_install_path) const {
    declare_output(interface, relative_install_path);
}

interface_phase_t::output_t interface_phase_t::installed_output() const {
    return output_t {
        .include_dirs = { install_dir() }
    };
}

library_phase_t::library_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("library", module, library_type, predecessor)
{
}

void library_phase_t::add_library(const filesystem::path_t& library, const filesystem::relative_path_t& relative_install_path) const {
    declare_output(library, relative_install_path);
}

library_phase_t::output_t library_phase_t::installed_output() const {
    return output_t {
        .libraries = filesystem::find(install_dir(), !filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_all)
    };
}

binary_phase_t::binary_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("binary", module, library_type, predecessor)
{
}

void binary_phase_t::add_binary(const filesystem::path_t& binary, const filesystem::relative_path_t& relative_install_path) const {
    declare_output(binary, relative_install_path);
}

binary_phase_t::output_t binary_phase_t::installed_output() const {
    const auto cli = install_dir() / filesystem::relative_path_t("cli");

    if (!filesystem::exists(cli)) {
        throw std::runtime_error(std::format("kernel::cpp_builder::builder::binary_phase_t::installed_output: expected module cli '{}' to exist", cli));
    }

    return output_t {
        .binary_root = install_dir(),
        .cli = cli
    };
}

config_phase_t::config_phase_t(graph::module_t& module, library_type_t library_type):
    phase_base_t("config", module, library_type, nullptr),
    source(module, library_type, this),
    interface(module, library_type, &source),
    library(module, library_type, &interface),
    binary(module, library_type, &library)
{
}

config_phase_t::output_t config_phase_t::installed_output() const {
    return output_t {
        .library_type = library_type()
    };
}

} // namespace builder

} // namespace cpp_builder

} // namespace kernel
