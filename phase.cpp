#include "phase.h"

#include "compiler.h"
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

filesystem::relative_path_t library_type_relative_dir(library_type_t library_type) {
    switch (library_type) {
        case library_type_t::STATIC: return filesystem::relative_path_t("static");
        case library_type_t::SHARED: return filesystem::relative_path_t("shared");
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::library_type_relative_dir: unknown library_type {}", static_cast<std::underlying_type_t<library_type_t>>(library_type)));
    }
}

} // namespace

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

graph::module_t& phase_base_t::module() const {
    return m_module;
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

void phase_base_t::declare_output(const filesystem::path_t& artifact, const filesystem::relative_path_t& relative_install_path) const {
    m_declared_outputs.push_back(declared_output_t {
        .artifact = artifact,
        .relative_install_path = relative_install_path
    });
}

std::string phase_base_t::producer_symbol_name() const {
    return std::format("phase__{}", m_name);
}

void phase_base_t::execute() const {
    auto& module = this->module();
    const auto builder_plugin = [&]() {
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
            phase_chain_t dependency_phase_chain(*dependency, library_type_t::SHARED);

            const auto dependency_interface_outputs = dependency_phase_chain.interface.materialize_all<interface_phase_t>();
            for (const auto& dependency_interface_output : dependency_interface_outputs) {
                include_dirs.insert(include_dirs.end(), dependency_interface_output.interfaces.begin(), dependency_interface_output.interfaces.end());
            }

            const auto dependency_library_outputs = dependency_phase_chain.library.materialize_all<library_phase_t>();
            for (const auto& dependency_library_output : dependency_library_outputs) {
                libraries.insert(libraries.end(), dependency_library_output.libraries.begin(), dependency_library_output.libraries.end());
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
            throw std::runtime_error(std::format("kernel::cpp_builder::builder::execute: expected builder plugin '{}' to exist", builder_plugin));
        }

        return builder_plugin;
    }();

    const auto dispatch = [&]<class phase_t>(const phase_t& phase) {
        shared_library::loader_t loader(builder_plugin, shared_library::lifetime_t::PROCESS, shared_library::symbol_resolution_t::LAZY, shared_library::symbol_visibility_t::LOCAL);
        using fn_t = void (*)(const phase_t*);
        fn_t fn = loader.resolve(producer_symbol_name().c_str());
        fn(&phase);
    };

    if (const auto* phase = dynamic_cast<const source_phase_t*>(this)) {
        dispatch(*phase);
    } else if (const auto* phase = dynamic_cast<const interface_phase_t*>(this)) {
        dispatch(*phase);
    } else if (const auto* phase = dynamic_cast<const library_phase_t*>(this)) {
        dispatch(*phase);
    } else if (const auto* phase = dynamic_cast<const binary_phase_t*>(this)) {
        dispatch(*phase);
    } else {
        throw std::runtime_error(std::format("kernel::cpp_builder::builder::phase_base_t::execute: unsupported phase '{}'", name()));
    }
}

source_phase_t::source_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("source", module, library_type, predecessor)
{
}

void source_phase_t::add_source(const filesystem::path_t& source, const filesystem::relative_path_t& relative_install_path) const {
    declare_output(source, relative_install_path);
}

interface_phase_t::interface_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("interface", module, library_type, predecessor)
{
}

void interface_phase_t::add_interface(const filesystem::path_t& interface, const filesystem::relative_path_t& relative_install_path) const {
    declare_output(interface, relative_install_path);
}

library_phase_t::library_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("library", module, library_type, predecessor)
{
}

void library_phase_t::add_library(const filesystem::path_t& library, const filesystem::relative_path_t& relative_install_path) const {
    declare_output(library, relative_install_path);
}

binary_phase_t::binary_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("binary", module, library_type, predecessor)
{
}

void binary_phase_t::add_binary(const filesystem::path_t& binary, const filesystem::relative_path_t& relative_install_path) const {
    declare_output(binary, relative_install_path);
}

phase_chain_t::phase_chain_t(graph::module_t& module, library_type_t library_type):
    source(module, library_type),
    interface(module, library_type, &source),
    library(module, library_type, &interface),
    binary(module, library_type, &library)
{
}

} // namespace builder

} // namespace cpp_builder

} // namespace kernel
