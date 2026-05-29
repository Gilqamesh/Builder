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

filesystem::relative_path_t library_type_relative_dir(library_type_t library_type) {
    switch (library_type) {
        case library_type_t::STATIC: return filesystem::relative_path_t("static");
        case library_type_t::SHARED: return filesystem::relative_path_t("shared");
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::library_type_relative_dir: unknown library_type {}", static_cast<std::underlying_type_t<library_type_t>>(library_type)));
    }
}

} // namespace

source_output_t materialized_output(const source_phase_t& phase) {
    return source_output_t {
        .source_root = phase.install_dir()
    };
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
    const auto binary_root = phase.install_dir();
    binary_output_t output {
        .binary_root = binary_root,
        .cli = binary_root / filesystem::relative_path_t("cli")
    };
    if (!filesystem::exists(output.cli)) {
        throw std::runtime_error(std::format("kernel::cpp_builder::builder::binary_phase_t::materialized_output: expected module cli '{}' to exist", output.cli));
    }
    return output;
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

std::string phase_base_t::producer_symbol_name() const {
    return std::format("phase__{}", m_name);
}

template <class phase_t>
void execute_producer_phase(const phase_t& phase) {
    auto& module = phase.module();
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
            module.builder_build_dir(),
            module.source_dir(),
            include_dirs,
            module.builder_path(),
            tool_path_defines(),
            libraries,
            builder_plugin
        );

        if (!filesystem::exists(builder_plugin)) {
            throw std::runtime_error(std::format("kernel::cpp_builder::builder::execute_producer_phase: expected builder plugin '{}' to exist", builder_plugin));
        }

        return builder_plugin;
    }();

    shared_library::loader_t loader(builder_plugin, shared_library::lifetime_t::PROCESS, shared_library::symbol_resolution_t::LAZY, shared_library::symbol_visibility_t::LOCAL);
    using fn_t = void (*)(const phase_t*);
    const auto producer_symbol_name = std::format("phase__{}", phase.name());
    fn_t fn = loader.resolve(producer_symbol_name.c_str());
    fn(&phase);
}

source_phase_t::source_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("source", module, library_type, predecessor)
{
}

void source_phase_t::execute() const {
    const auto module_source_dir = module().source_dir();
    for (const auto& source_path : filesystem::find(module_source_dir, !filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_all)) {
        const auto target_path = install_dir() / module_source_dir.relative(source_path);
        const auto target_path_parent = target_path.parent();
        if (!filesystem::exists(target_path_parent)) {
            filesystem::create_directories(target_path_parent);
        }
        filesystem::copy(source_path, target_path);
    }
}

interface_phase_t::interface_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("interface", module, library_type, predecessor)
{
}

void interface_phase_t::execute() const {
    execute_producer_phase(*this);
}

library_phase_t::library_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("library", module, library_type, predecessor)
{
}

void library_phase_t::execute() const {
    execute_producer_phase(*this);
}

binary_phase_t::binary_phase_t(graph::module_t& module, library_type_t library_type, const iphase_t* predecessor):
    phase_base_t("binary", module, library_type, predecessor)
{
}

void binary_phase_t::execute() const {
    execute_producer_phase(*this);
}

phase_chain_t::phase_chain_t(graph::module_t& module, library_type_t library_type):
    source(module, library_type),
    interface(module, library_type, &source),
    library(module, library_type, &interface),
    binary(module, library_type, &library)
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
