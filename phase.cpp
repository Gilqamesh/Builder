#include "phase.h"

#include "compiler.h"

#include <format>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace kernel {

namespace phase {

static output_artifact_t find_output_artifact(
    const source_phase_t::output_t& output,
    const filesystem::relative_path_t& relative_path
) {
    const output_artifact_t* result = nullptr;

    for (const auto& artifact : output.artifacts) {
        if (artifact.relative_path == relative_path) {
            if (result != nullptr) {
                throw std::runtime_error(std::format("kernel::phase::find_output_artifact: output has more than one artifact at relative path '{}'", relative_path));
            }

            result = &artifact;
        }
    }

    if (result == nullptr) {
        throw std::runtime_error(std::format("kernel::phase::find_output_artifact: output has no artifact at relative path '{}'", relative_path));
    }

    return *result;
}

static filesystem::relative_path_t library_type_relative_dir(module_config::library_type_t library_type) {
    switch (library_type) {
        case module_config::library_type_t::STATIC: return filesystem::relative_path_t("static");
        case module_config::library_type_t::SHARED: return filesystem::relative_path_t("shared");
        default: throw std::runtime_error(std::format("kernel::phase::library_type_relative_dir: unknown library_type {}", static_cast<std::underlying_type_t<module_config::library_type_t>>(library_type)));
    }
}

template <class phase_t>
static std::vector<typename phase_t::output_t> build_closure_for_module(
    graph::module_t& module,
    module_config::module_config_t module_config
) {
    output_artifacts_t output {
        .root = filesystem::path_t("/"),
        .artifacts = {}
    };
    const phase_t requested_phase(module, module_config, output);
    return requested_phase.template build_closure<phase_t>();
}

static std::vector<binding::binding_t> tool_path_defines() {
    return {
        binding::binding_t("KERNEL_CXX_COMPILER_PATH", KERNEL_CXX_COMPILER_PATH),
        binding::binding_t("KERNEL_CC_COMPILER_PATH", KERNEL_CC_COMPILER_PATH),
        binding::binding_t("KERNEL_AR_PATH", KERNEL_AR_PATH)
    };
}

static filesystem::path_t builder_path(const graph::module_t& module) {
    return module.source_dir() / filesystem::relative_path_t(graph::BUILDER_CPP);
}

static filesystem::path_t builder_dir(const graph::module_t& module) {
    return module.artifact_dir() / filesystem::relative_path_t("builder");
}

static filesystem::path_t builder_build_dir(const graph::module_t& module) {
    return builder_dir(module) / filesystem::relative_path_t("build");
}

static filesystem::path_t builder_install_dir(const graph::module_t& module) {
    return builder_dir(module) / filesystem::relative_path_t("install");
}

static filesystem::path_t builder_install_path(const graph::module_t& module) {
    return builder_install_dir(module) / filesystem::relative_path_t("builder.so");
}

static filesystem::path_t builder_install_latest_path(const graph::module_t& module) {
    return module.artifact_latest_dir() / filesystem::relative_path_t("builder/install/builder.so");
}

phase_base_t::phase_base_t(
    std::string_view name,
    graph::module_t& module,
    module_config::module_config_t module_config,
    output_artifacts_t& output
):
    m_name(name),
    m_module(module),
    m_module_config(module_config),
    m_output(output)
{
}

std::string_view phase_base_t::name() const {
    return m_name;
}

graph::module_t& phase_base_t::module() const {
    return m_module;
}

module_config::module_config_t phase_base_t::module_config() const {
    return m_module_config;
}

filesystem::path_t phase_base_t::artifact_dir() const {
    return m_module.artifact_dir() / filesystem::relative_path_t(std::string(m_name));
}

filesystem::path_t phase_base_t::build_dir() const {
    return artifact_dir() / filesystem::relative_path_t("build") / library_type_relative_dir(module_config().library_type);
}

filesystem::path_t phase_base_t::install_dir() const {
    return artifact_dir() / filesystem::relative_path_t("install") / library_type_relative_dir(module_config().library_type);
}

output_artifacts_t& phase_base_t::output() const {
    return m_output;
}

filesystem::path_t phase_base_t::builder_plugin() const {
    const auto plugin_path = builder_install_path(m_module);
    const auto build_dir = builder_build_dir(m_module);
    const auto install_dir = builder_install_dir(m_module);
    const auto marker_path = [&](std::string_view state) {
        return build_dir / filesystem::relative_path_t(std::format("builder.{}", state));
    };
    const auto started_marker = marker_path("started");
    const auto complete_marker = marker_path("complete");

    if (filesystem::exists(complete_marker)) {
        if (!filesystem::exists(plugin_path)) {
            throw std::runtime_error(std::format("kernel::phase::phase_base_t::builder_plugin: completed builder plugin '{}' does not exist", plugin_path));
        }

        return plugin_path;
    }

    if (filesystem::exists(started_marker)) {
        throw std::runtime_error(std::format("kernel::phase::phase_base_t::builder_plugin: re-entry detected for builder plugin '{}'", plugin_path));
    }

    if (&m_module == m_module.workspace->workspace_ecosystem->this_module) {
        const auto latest_builder_plugin = builder_install_latest_path(m_module);
        if (filesystem::exists(latest_builder_plugin)) {
            return latest_builder_plugin;
        }

#ifdef KERNEL_BOOTSTRAP_BUILDER_PLUGIN_PATH
        const auto bootstrap_builder_plugin = filesystem::path_t(KERNEL_BOOTSTRAP_BUILDER_PLUGIN_PATH);
        if (filesystem::exists(bootstrap_builder_plugin)) {
            return bootstrap_builder_plugin;
        }
#endif
    }

    if (filesystem::exists(build_dir)) {
        filesystem::remove_all(build_dir);
    }
    if (filesystem::exists(install_dir)) {
        filesystem::remove_all(install_dir);
    }

    try {
        filesystem::create_directories(build_dir);
        filesystem::touch(started_marker);

        std::vector<filesystem::path_t> include_dirs;
        std::vector<filesystem::path_t> libraries;

        for (auto* dependency : m_module.builder_dependencies) {
            for (const auto& dependency_include_dirs : build_closure_for_module<interface_phase_t>(*dependency, module_config::module_config_t { .library_type = module_config::library_type_t::SHARED })) {
                include_dirs.push_back(dependency_include_dirs.root);
            }

            for (const auto& dependency_libraries : build_closure_for_module<library_phase_t>(*dependency, module_config::module_config_t { .library_type = module_config::library_type_t::SHARED })) {
                for (const auto& library : dependency_libraries.artifacts) {
                    libraries.push_back(library.path);
                }
            }
        }

        compiler::link_inputs_t link_inputs;
        if (!libraries.empty()) {
            link_inputs.groups.push_back(compiler::link_input_group_t {
                .libraries = libraries,
                .static_library_group = false
            });
        }

        compiler::create_library(
            build_dir,
            m_module.source_dir(),
            include_dirs,
            { builder_path(m_module) },
            tool_path_defines(),
            compiler::library_type_t::SHARED,
            link_inputs,
            plugin_path
        );

        if (!filesystem::exists(plugin_path)) {
            throw std::runtime_error(std::format("kernel::phase::phase_base_t::builder_plugin: expected builder plugin '{}' to exist", plugin_path));
        }

        filesystem::touch(complete_marker);
        filesystem::remove(started_marker);

        return plugin_path;
    } catch (...) {
        filesystem::remove_all(build_dir);
        filesystem::remove_all(install_dir);
        throw ;
    }
}

source_phase_t::source_phase_t(
    graph::module_t& module,
    module_config::module_config_t module_config,
    output_artifacts_t& output
):
    phase_base_t("source", module, module_config, output)
{
}

filesystem::path_t source_phase_t::source_dir() const {
    return module().source_dir();
}

void source_phase_t::add_source_tree() const {
    const auto source_root = source_dir();

    for (const auto& source : filesystem::find(source_root, !filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_all)) {
        add_source(source, source_root.relative(source));
    }
}

void source_phase_t::add_source(const filesystem::path_t& source, const filesystem::relative_path_t& relative_install_path) const {
    output().artifacts.push_back(output_artifact_t {
        .path = source,
        .relative_path = relative_install_path
    });
}

interface_phase_t::interface_phase_t(
    graph::module_t& module,
    module_config::module_config_t module_config,
    output_artifacts_t& output
):
    phase_base_t("interface", module, module_config, output)
{
}

void interface_phase_t::add_headers_from_source() const {
    const auto source_output = build<source_phase_t>();

    for (const auto& artifact : source_output.artifacts) {
        if (filesystem::find_include_predicate_t::h_file(artifact.path) || filesystem::find_include_predicate_t::hpp_file(artifact.path)) {
            add_interface(artifact);
        }
    }
}

void interface_phase_t::add_interface_from_source(const filesystem::relative_path_t& relative_path) const {
    add_interface(find_output_artifact(build<source_phase_t>(), relative_path));
}

void interface_phase_t::add_interface(const filesystem::path_t& interface, const filesystem::relative_path_t& module_relative_install_path) const {
    const auto relative_install_path = filesystem::relative_path_t(
        filesystem::relative_path_t("builder").to_native_path()
        / module().module_relative_path_to_workspace.to_native_path()
        / module_relative_install_path.to_native_path()
    );

    output().artifacts.push_back(output_artifact_t {
        .path = interface,
        .relative_path = relative_install_path
    });
}

void interface_phase_t::add_interface(const output_artifact_t& artifact) const {
    add_interface(artifact.path, artifact.relative_path);
}

library_phase_t::library_phase_t(
    graph::module_t& module,
    module_config::module_config_t module_config,
    output_artifacts_t& output
):
    phase_base_t("library", module, module_config, output)
{
}

void library_phase_t::add_library(const output_artifact_t& artifact) const {
    add_library(artifact.path, artifact.relative_path);
}

void library_phase_t::add_library(const filesystem::path_t& library, const filesystem::relative_path_t& relative_install_path) const {
    output().artifacts.push_back(output_artifact_t {
        .path = library,
        .relative_path = relative_install_path
    });
}

binary_phase_t::binary_phase_t(
    graph::module_t& module,
    module_config::module_config_t module_config,
    output_artifacts_t& output
):
    phase_base_t("binary", module, module_config, output)
{
}

void binary_phase_t::add_cli(const filesystem::path_t& binary) const {
    output().artifacts.push_back(output_artifact_t {
        .path = binary,
        .relative_path = filesystem::relative_path_t("cli")
    });
}

void binary_phase_t::add_binary(const output_artifact_t& artifact) const {
    add_binary(artifact.path, artifact.relative_path);
}

void binary_phase_t::add_binary(const filesystem::path_t& binary, const filesystem::relative_path_t& relative_install_path) const {
    if (relative_install_path == filesystem::relative_path_t("cli")) {
        throw std::runtime_error("kernel::phase::binary_phase_t::add_binary: use add_cli to publish the default CLI artifact");
    }

    output().artifacts.push_back(output_artifact_t {
        .path = binary,
        .relative_path = relative_install_path
    });
}

} // namespace phase

} // namespace kernel
