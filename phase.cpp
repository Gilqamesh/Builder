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
    m_predecessor(predecessor),
    m_output {
        .root = install_dir(),
        .artifacts = {}
    }
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
            include_dirs.push_back(dependency_include_dirs.root);
        }

        for (const auto& dependency_libraries : dependency->materialize_all<library_phase_t>()) {
            for (const auto& library : dependency_libraries.artifacts) {
                libraries.push_back(library.path);
            }
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
    m_output.artifacts.push_back(output_artifact_t {
        .path = source,
        .relative_path = relative_install_path
    });
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
    m_output.artifacts.push_back(output_artifact_t {
        .path = interface,
        .relative_path = relative_install_path
    });
}

library_phase_t::library_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("library", module, library_type, predecessor)
{
}

void library_phase_t::add_library(const filesystem::path_t& library, const filesystem::relative_path_t& relative_install_path) const {
    m_output.artifacts.push_back(output_artifact_t {
        .path = library,
        .relative_path = relative_install_path
    });
}

binary_phase_t::binary_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("binary", module, library_type, predecessor)
{
}

void binary_phase_t::add_binary(const filesystem::path_t& binary, const filesystem::relative_path_t& relative_install_path) const {
    m_output.artifacts.push_back(output_artifact_t {
        .path = binary,
        .relative_path = relative_install_path
    });
}

config_phase_t::config_phase_t(graph::module_t& module, library_type_t library_type):
    phase_base_t("config", module, library_type, nullptr),
    source(module, library_type, this),
    interface(module, library_type, &source),
    library(module, library_type, &interface),
    binary(module, library_type, &library)
{
}

} // namespace builder

} // namespace cpp_builder

} // namespace kernel
