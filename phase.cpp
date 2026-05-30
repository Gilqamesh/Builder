#include "phase.h"

#include <format>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace kernel {

namespace cpp_builder {

namespace builder {

static filesystem::relative_path_t library_type_relative_dir(library_type_t library_type) {
    switch (library_type) {
        case library_type_t::STATIC: return filesystem::relative_path_t("static");
        case library_type_t::SHARED: return filesystem::relative_path_t("shared");
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::library_type_relative_dir: unknown library_type {}", static_cast<std::underlying_type_t<library_type_t>>(library_type)));
    }
}

phase_base_t::phase_base_t(std::string_view name, graph::module_t& module, library_type_t configured_library_type):
    m_name(name),
    m_module(module),
    m_library_type(configured_library_type),
    m_output {
        .root = install_dir(),
        .artifacts = {}
    }
{
}

std::string_view phase_base_t::name() const {
    return m_name;
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

source_phase_t::source_phase_t(graph::module_t& module, library_type_t library_type):
    phase_base_t("source", module, library_type)
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

interface_phase_t::interface_phase_t(graph::module_t& module, library_type_t library_type):
    phase_base_t("interface", module, library_type)
{
}

void interface_phase_t::add_interface(const filesystem::path_t& interface, const filesystem::relative_path_t& relative_install_path) const {
    m_output.artifacts.push_back(output_artifact_t {
        .path = interface,
        .relative_path = relative_install_path
    });
}

library_phase_t::library_phase_t(graph::module_t& module, library_type_t library_type):
    phase_base_t("library", module, library_type)
{
}

void library_phase_t::add_library(const filesystem::path_t& library, const filesystem::relative_path_t& relative_install_path) const {
    m_output.artifacts.push_back(output_artifact_t {
        .path = library,
        .relative_path = relative_install_path
    });
}

binary_phase_t::binary_phase_t(graph::module_t& module, library_type_t library_type):
    phase_base_t("binary", module, library_type)
{
}

void binary_phase_t::add_binary(const filesystem::path_t& binary, const filesystem::relative_path_t& relative_install_path) const {
    m_output.artifacts.push_back(output_artifact_t {
        .path = binary,
        .relative_path = relative_install_path
    });
}

config_phase_t::config_phase_t(graph::module_t& module, library_type_t library_type):
    phase_base_t("config", module, library_type),
    source(module, library_type),
    interface(module, library_type),
    library(module, library_type),
    binary(module, library_type)
{
}

} // namespace builder

} // namespace cpp_builder

} // namespace kernel
