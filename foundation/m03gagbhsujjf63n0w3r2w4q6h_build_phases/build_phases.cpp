#include "build_phases.h"

#include <m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain/cxx_toolchain.h>
#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
#include <m03gagbhsyhlx2pk5sdabbr1sx_signal_handler/signal_handler.h>
#include <m03gagbhsx4j5z28bqkac3dhhh_shared_library/shared_library.h>

#include <format>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>

#ifndef M03GAGBHSUJJF63N0W3R2W4Q6H_BUILD_PHASES_BOOTSTRAP_BUILDER_PLUGIN_PATH
# error M03GAGBHSUJJF63N0W3R2W4Q6H_BUILD_PHASES_BOOTSTRAP_BUILDER_PLUGIN_PATH must be defined by bootstrap
#endif

namespace m03gagbhsujjf63n0w3r2w4q6h_build_phases {

static m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t library_type_relative_dir(m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t library_type) {
    switch (library_type) {
        case m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t::STATIC: return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("static");
        case m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t::SHARED: return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("shared");
        default: throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::library_type_relative_dir: unknown library_type {}", static_cast<std::underlying_type_t<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t>>(library_type)));
    }
}

static std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> include_dirs_from_outputs(const std::vector<interface_phase_t::installed_t>& interfaces) {
    std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> include_dirs;
    include_dirs.reserve(interfaces.size());

    for (const auto& interface_output : interfaces) {
        include_dirs.push_back(interface_output.root());
    }

    return include_dirs;
}

static std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t> compiler_source_files(const std::vector<phase_base_t::built_t>& source_files) {
    std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t> result;
    result.reserve(source_files.size());

    for (const auto& source_file : source_files) {
        result.push_back(source_file.rooted_path());
    }

    return result;
}

static std::string cxx_string_literal(std::string_view value) {
    std::string result("\"");

    for (const char c : value) {
        if (c == '\\' || c == '"') {
            result.push_back('\\');
        }
        result.push_back(c);
    }

    result.push_back('"');
    return result;
}

static m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t module_library_relative_output_path(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t& module_name,
    m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t library_type
) {
    switch (library_type) {
        case m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t::STATIC:
            return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(std::format("lib{}.a", module_name));
        case m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t::SHARED:
            return m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(std::format("lib{}.so", module_name));
        default:
            throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::module_library_relative_output_path: unknown library_type {}", static_cast<std::underlying_type_t<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t>>(library_type)));
    }
}

static m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::link_inputs_t binary_link_inputs(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    build_config_t build_config
) {
    bool static_libraries = false;
    switch (build_config.library_type) {
        case m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t::STATIC: static_libraries = true; break ;
        case m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t::SHARED: break ;
        default: throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_link_inputs: unknown library_type {}", static_cast<std::underlying_type_t<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t>>(build_config.library_type)));
    }

    const auto closure_groups = module.closure_groups();

    m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::link_inputs_t result;
    for (auto group_it = closure_groups.rbegin(); group_it != closure_groups.rend(); ++group_it) {
        m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::link_input_group_t group {
            .libraries = {},
            .static_library_group = false
        };

        for (auto module_it = group_it->rbegin(); module_it != group_it->rend(); ++module_it) {
            const auto phase = phase_base_t::make(**module_it, build_config);
            const auto libraries = phase->install<library_phase_t>();
            for (const auto& library : m03gagbhsnusi43zogoacgj2ez_filesystem::find(libraries.root(), !m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t::is_dir, m03gagbhsnusi43zogoacgj2ez_filesystem::find_descend_predicate_t::descend_all)) {
                group.libraries.push_back(library.path());
            }
        }

        if (!group.libraries.empty()) {
            group.static_library_group = static_libraries && 1 < group.libraries.size();
            result.groups.push_back(group);
        }
    }

    return result;
}

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t builder_dir(const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module) {
    return module.artifact_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("builder");
}

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t builder_build_dir(const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module) {
    return builder_dir(module) / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("build");
}

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t builder_install_dir(const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module) {
    return builder_dir(module) / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("install");
}

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t builder_install_path(const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module) {
    return builder_install_dir(module) / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("builder.so");
}

static m03gagbhsnusi43zogoacgj2ez_filesystem::path_t bootstrap_builder_plugin_path() {
    const auto path = m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(M03GAGBHSUJJF63N0W3R2W4Q6H_BUILD_PHASES_BOOTSTRAP_BUILDER_PLUGIN_PATH);
    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(path)) {
        return path;
    }

    throw std::runtime_error(std::format(
        "m03gagbhsujjf63n0w3r2w4q6h_build_phases::bootstrap_builder_plugin_path: compiled bootstrap builder plugin '{}' does not exist; run 'make bootstrap' to recreate bootstrap artifacts",
        path
    ));
}

phase_base_t::built_t::built_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& rooted_path):
    m_rooted_path(rooted_path)
{
}

const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& phase_base_t::built_t::rooted_path() const {
    return m_rooted_path;
}

phase_base_t::phase_base_t(
    std::string_view name,
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    build_config_t build_config,
    std::unique_ptr<phase_base_t> previous_phase
):
    m_name(name),
    m_module(module),
    m_build_config(build_config),
    m_previous_phase(std::move(previous_phase))
{
}

std::unique_ptr<phase_base_t> phase_base_t::make(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    build_config_t build_config
) {
    std::unique_ptr<phase_base_t> phase;
    std::unordered_set<phase_id_t> phase_ids;

    for (const auto phase_id : build_config.phase_order) {
        if (!phase_ids.insert(phase_id).second) {
            throw std::runtime_error(std::format(
                "m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::make: duplicate phase id {} in build_config.phase_order",
                static_cast<std::underlying_type_t<phase_id_t>>(phase_id)
            ));
        }

        switch (phase_id) {
            case phase_id_t::SOURCE:
                phase = std::make_unique<source_phase_t>(module, build_config, std::move(phase));
                break ;
            case phase_id_t::INTERFACE:
                phase = std::make_unique<interface_phase_t>(module, build_config, std::move(phase));
                break ;
            case phase_id_t::LIBRARY:
                phase = std::make_unique<library_phase_t>(module, build_config, std::move(phase));
                break ;
            case phase_id_t::BINARY:
                phase = std::make_unique<binary_phase_t>(module, build_config, std::move(phase));
                break ;
            default:
                throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::make: unsupported phase id {}", static_cast<std::underlying_type_t<phase_id_t>>(phase_id)));
        }
    }

    if (!phase) {
        throw std::runtime_error("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::make: build_config.phase_order is empty");
    }

    return phase;
}

std::string_view phase_base_t::name() const {
    return m_name;
}

m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& phase_base_t::module() const {
    return m_module;
}

build_config_t phase_base_t::build_config() const {
    return m_build_config;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t phase_base_t::artifact_dir() const {
    return m_module.artifact_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(std::string(m_name));
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t phase_base_t::build_dir() const {
    return artifact_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("build") / library_type_relative_dir(build_config().library_type);
}

phase_base_t::built_t phase_base_t::build(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) const {
    for (const auto* phase = previous_phase(); phase != nullptr; phase = phase->previous_phase()) {
        const auto previous_install_dir = phase->install_dir();
        if (previous_install_dir.is_child(path)) {
            return build(m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t(previous_install_dir, previous_install_dir.relative(path)));
        }
    }

    throw std::runtime_error(std::format(
        "m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::build: build input '{}' is not under an earlier phase install_dir for phase '{}'",
        path,
        name()
    ));
}

phase_base_t::built_t phase_base_t::build(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& rooted_path) const {
    for (const auto* phase = previous_phase(); phase != nullptr; phase = phase->previous_phase()) {
        if (rooted_path.root() == phase->install_dir()) {
            return built_t(rooted_path);
        }
    }

    throw std::runtime_error(std::format(
        "m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::build: selected input root '{}' is not an installed root of a previous phase for phase '{}'",
        rooted_path.root(),
        name()
    ));
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t phase_base_t::install_dir() const {
    return artifact_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("install") / library_type_relative_dir(build_config().library_type);
}

void phase_base_t::install_as(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& relative_path
) const {
    const auto installed_artifact = install_dir() / relative_path;

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(installed_artifact)) {
        throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::install_as: phase '{}' already has an artifact at relative path '{}'", name(), install_dir().relative(installed_artifact)));
    }

    m03gagbhsnusi43zogoacgj2ez_filesystem::copy(path, installed_artifact);
}

const phase_base_t* phase_base_t::previous_phase() const {
    return m_previous_phase.get();
}

m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t phase_base_t::installed_relative_path(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) const {
    const auto build_dir = this->build_dir();
    if (build_dir.is_child(path)) {
        return build_dir.relative(path);
    }

    for (const auto* phase = previous_phase(); phase != nullptr; phase = phase->previous_phase()) {
        const auto previous_install_dir = phase->install_dir();
        if (previous_install_dir.is_child(path)) {
            return previous_install_dir.relative(path);
        }
    }

    throw std::runtime_error(std::format(
        "m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::installed_relative_path: artifact '{}' is not under phase '{}' build_dir or an earlier phase install_dir",
        path,
        name()
    ));
}

void phase_base_t::finalize_install() const {
}

void phase_base_t::install(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path) const {
    install_as(path, installed_relative_path(path));
}

void phase_base_t::install(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& rooted_path) const {
    if (rooted_path.root() == build_dir()) {
        install_as(rooted_path.path(), rooted_path.relative_path());
        return ;
    }

    for (const auto* phase = previous_phase(); phase != nullptr; phase = phase->previous_phase()) {
        if (rooted_path.root() == phase->install_dir()) {
            install_as(rooted_path.path(), rooted_path.relative_path());
            return ;
        }
    }

    throw std::runtime_error(std::format(
        "m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::install: selected path root '{}' is not phase '{}' build_dir or an earlier phase install_dir",
        rooted_path.root(),
        name()
    ));
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t phase_base_t::builder_plugin() const {
    const auto plugin_path = builder_install_path(m_module);
    const auto build_dir = builder_build_dir(m_module);
    const auto install_dir = builder_install_dir(m_module);
    const auto marker_path = [&](std::string_view state) {
        return build_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(std::format("builder.{}", state));
    };
    const auto started_marker = marker_path("started");
    const auto complete_marker = marker_path("complete");

    if (m_module.workspace().graph().is_active_builder_bootstrap_module(m_module)) {
        return bootstrap_builder_plugin_path();
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(complete_marker)) {
        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(plugin_path)) {
            throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::builder_plugin: completed builder plugin '{}' does not exist", plugin_path));
        }

        return plugin_path;
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(started_marker)) {
        throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::builder_plugin: re-entry detected for builder plugin '{}'", plugin_path));
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(build_dir)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove_all(build_dir);
    }
    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(install_dir)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove_all(install_dir);
    }

    try {
        {
            m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::scoped_termination_guard_t termination_guard;

            m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(build_dir);
            m03gagbhsnusi43zogoacgj2ez_filesystem::touch(started_marker);

            std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> include_dirs;
            std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> libraries;

            for (auto* dependency : m_module.builder_dependencies()) {
                const auto dependency_phase = phase_base_t::make(
                    *dependency,
                    build_config_t { .library_type = m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t::SHARED }
                );

                for (const auto& dependency_include_dirs : dependency_phase->install_closure<interface_phase_t>()) {
                    include_dirs.push_back(dependency_include_dirs.root());
                }

                for (const auto& dependency_libraries : dependency_phase->install_closure<library_phase_t>()) {
                    for (const auto& library : m03gagbhsnusi43zogoacgj2ez_filesystem::find(dependency_libraries.root(), !m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t::is_dir, m03gagbhsnusi43zogoacgj2ez_filesystem::find_descend_predicate_t::descend_all)) {
                        libraries.push_back(library.path());
                    }
                }
            }

            m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::link_inputs_t link_inputs;
            if (!libraries.empty()) {
                link_inputs.groups.push_back(m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::link_input_group_t {
                    .libraries = libraries,
                    .static_library_group = false
                });
            }

            m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_library(
                build_dir,
                include_dirs,
                {
                    m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t(
                        m_module.source_dir(),
                        m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::BUILDER_CPP)
                    )
                },
                {},
                m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t::SHARED,
                link_inputs,
                plugin_path
            );
        }

        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(plugin_path)) {
            throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::builder_plugin: expected builder plugin '{}' to exist", plugin_path));
        }

        m03gagbhsnusi43zogoacgj2ez_filesystem::touch(complete_marker);
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(started_marker);

        return plugin_path;
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove_all(build_dir);
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove_all(install_dir);
        throw ;
    }
}

source_phase_t::source_phase_t(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    build_config_t build_config,
    std::unique_ptr<phase_base_t> previous_phase
):
    phase_base_t("source", module, build_config, std::move(previous_phase))
{
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t source_phase_t::source_dir() const {
    return module().source_dir();
}

source_phase_t::installed_t::installed_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root):
    m_root(root)
{
}

const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source_phase_t::installed_t::root() const {
    return m_root;
}

void source_phase_t::install_source_tree() const {
    const auto source_root = source_dir();

    for (const auto& source : m03gagbhsnusi43zogoacgj2ez_filesystem::find(source_root, !m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t::is_dir, m03gagbhsnusi43zogoacgj2ez_filesystem::find_descend_predicate_t::descend_all)) {
        install_as(source.path(), source.relative_path());
    }
}

void source_phase_t::install_source(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source) const {
    install(source);
}

interface_phase_t::interface_phase_t(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    build_config_t build_config,
    std::unique_ptr<phase_base_t> previous_phase
):
    phase_base_t("interface", module, build_config, std::move(previous_phase))
{
}

interface_phase_t::installed_t::installed_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root):
    m_root(root)
{
}

const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& interface_phase_t::installed_t::root() const {
    return m_root;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t interface_phase_t::build_interface_as(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& relative_path
) const {
    const auto built_source = build(source);
    const auto staged_interface = build_dir() / relative_path;
    m03gagbhsnusi43zogoacgj2ez_filesystem::copy(built_source.rooted_path().path(), staged_interface);
    return staged_interface;
}

void interface_phase_t::install_headers_from_source() const {
    const auto sources = install<source_phase_t>();

    for (const auto& artifact : m03gagbhsnusi43zogoacgj2ez_filesystem::find(
        sources.root(),
        m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t::h_file || m03gagbhsnusi43zogoacgj2ez_filesystem::find_include_predicate_t::hpp_file,
        m03gagbhsnusi43zogoacgj2ez_filesystem::find_descend_predicate_t::descend_all
    )) {
        install_interface(artifact);
    }
}

void interface_phase_t::install_interface(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& interface) const {
    const auto relative_path = installed_relative_path(interface);
    const auto canonical_path = install_dir()
        / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(module().name().string())
        / relative_path;
    install_as(interface, install_dir().relative(canonical_path));
}

void interface_phase_t::install_interface(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& interface) const {
    install_interface(interface.path());
}

void interface_phase_t::install_interface_compatibility(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& interface) const {
    install(interface);
}

void interface_phase_t::install_interface_compatibility(const m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t& interface) const {
    install_interface_compatibility(interface.path());
}

library_phase_t::library_phase_t(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    build_config_t build_config,
    std::unique_ptr<phase_base_t> previous_phase
):
    phase_base_t("library", module, build_config, std::move(previous_phase))
{
}

library_phase_t::installed_t::installed_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root):
    m_root(root)
{
}

const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& library_phase_t::installed_t::root() const {
    return m_root;
}

m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t library_phase_t::library_type() const {
    return build_config().library_type;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t library_phase_t::build_library(
    const std::vector<phase_base_t::built_t>& source_files,
    const std::vector<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t>& defines
) const {
    const auto interfaces = install_closure<interface_phase_t>();
    const auto relative_output_path = module_library_relative_output_path(module().name(), library_type());

    return m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_library(
        build_dir(),
        include_dirs_from_outputs(interfaces),
        compiler_source_files(source_files),
        defines,
        library_type(),
        m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::link_inputs_t {},
        build_dir() / relative_output_path
    );
}

void library_phase_t::install_library(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& library) const {
    install(library);
}

binary_phase_t::binary_phase_t(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    build_config_t build_config,
    std::unique_ptr<phase_base_t> previous_phase
):
    phase_base_t("binary", module, build_config, std::move(previous_phase))
{
}

binary_phase_t::installed_t::installed_t(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root):
    m_root(root),
    m_cli(root / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("cli"))
{
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(m_cli)) {
        throw std::runtime_error("m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t::installed_t: binary phase did not publish a default CLI artifact");
    }
}

const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& binary_phase_t::installed_t::root() const {
    return m_root;
}

const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& binary_phase_t::installed_t::cli() const {
    return m_cli;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t binary_phase_t::build_cli(
    const std::vector<phase_base_t::built_t>& source_files,
    const std::vector<m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::define_t>& defines
) const {
    const auto interfaces = install_closure<interface_phase_t>();
    const auto link_inputs = binary_link_inputs(
        module(),
        build_config()
    );

    return m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_binary(
        build_dir(),
        include_dirs_from_outputs(interfaces),
        compiler_source_files(source_files),
        defines,
        link_inputs,
        build_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("cli")
    );
}

void binary_phase_t::install_cli(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& binary) const {
    const auto binary_build_dir = build_dir();

    if (!binary_build_dir.is_child(binary)) {
        throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t::install_cli: CLI artifact '{}' is not under binary phase build_dir '{}'", binary, binary_build_dir));
    }

    install_as(binary, m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("cli"));
}

void binary_phase_t::install_binary(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& binary) const {
    const auto relative_install_path = installed_relative_path(binary);
    if (relative_install_path == m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("cli")) {
        throw std::runtime_error("m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t::install_binary: use install_cli to publish the default CLI artifact");
    }

    install_as(binary, relative_install_path);
}

void binary_phase_t::finalize_install() const {
    const auto installed_cli = install_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("cli");
    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(installed_cli)) {
        return ;
    }

    const auto source_relative_path = m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("default_cli.cpp");
    const auto source_path = build_dir() / source_relative_path;
    {
        std::ofstream ofs(source_path.string(), std::ios::binary | std::ios::trunc);
        if (!ofs) {
            throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t::finalize_install: failed to write default CLI source '{}'", source_path));
        }

        ofs
            << "#include <iostream>\n"
            << "\n"
            << "int main() {\n"
            << "    std::cout << " << cxx_string_literal(module().name().string()) << " << std::endl;\n"
            << "    return 0;\n"
            << "}\n";
        if (!ofs) {
            throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t::finalize_install: failed to write default CLI source '{}'", source_path));
        }
    }

    const auto binary = m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::build_binary(
        build_dir(),
        {},
        { m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t(build_dir(), source_relative_path) },
        {},
        m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::link_inputs_t {},
        build_dir() / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t("default_cli")
    );
    install_cli(binary);
}

template <class phase_t>
typename phase_t::installed_t phase_base_t::install(const phase_t& requested_phase) const {
    const auto build_dir = requested_phase.build_dir();
    const auto install_dir = requested_phase.install_dir();
    const auto marker_path = [&](std::string_view state) {
        return build_dir / m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t(std::format("{}.{}", requested_phase.name(), state));
    };
    const auto started_marker = marker_path("started");
    const auto complete_marker = marker_path("complete");

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(complete_marker)) {
        return typename phase_t::installed_t(requested_phase.install_dir());
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(started_marker)) {
        throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::install: re-entry detected for phase '{}'", requested_phase.name()));
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(build_dir)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove_all(build_dir);
    }
    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(install_dir)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove_all(install_dir);
    }

    try {
        {
            m03gagbhsyhlx2pk5sdabbr1sx_signal_handler::scoped_termination_guard_t termination_guard;

            if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(build_dir)) {
                m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(build_dir);
            }
            m03gagbhsnusi43zogoacgj2ez_filesystem::touch(started_marker);
            m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(install_dir);

            m03gagbhsx4j5z28bqkac3dhhh_shared_library::loader_t loader(
                requested_phase.builder_plugin(),
                m03gagbhsx4j5z28bqkac3dhhh_shared_library::lifetime_t::PROCESS,
                m03gagbhsx4j5z28bqkac3dhhh_shared_library::symbol_resolution_t::LAZY,
                m03gagbhsx4j5z28bqkac3dhhh_shared_library::symbol_visibility_t::LOCAL
            );
            const auto symbol_name = std::format("phase__{}", requested_phase.name());
            using fn_t = void (*)(const phase_t*);
            fn_t fn = loader.resolve(symbol_name.c_str());
            fn(&requested_phase);
            static_cast<const phase_base_t&>(requested_phase).finalize_install();
        }

        typename phase_t::installed_t installed_result(requested_phase.install_dir());

        const auto phase_artifact_dir = requested_phase.artifact_dir();
        const auto latest_dir = m_module.artifact_latest_dir();
        const auto latest_stage_dir = latest_dir / m_module.artifact_dir().relative(phase_artifact_dir);
        const auto latest_stage_tmp_dir = latest_stage_dir + "_tmp";

        if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(latest_stage_tmp_dir)) {
            m03gagbhsnusi43zogoacgj2ez_filesystem::remove_all(latest_stage_tmp_dir);
        }

        if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(latest_dir)) {
            m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(latest_dir);
        }

        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directory_symlink(phase_artifact_dir, latest_stage_tmp_dir);
        m03gagbhsnusi43zogoacgj2ez_filesystem::rename_replace(latest_stage_tmp_dir, latest_stage_dir);

        m03gagbhsnusi43zogoacgj2ez_filesystem::touch(complete_marker);
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(started_marker);

        return installed_result;
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove_all(build_dir);
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove_all(install_dir);
        throw ;
    }
}

template <class phase_t>
typename phase_t::installed_t phase_base_t::install() const {
    for (const auto* phase = this; phase != nullptr; phase = phase->previous_phase()) {
        if (const auto* requested_phase = dynamic_cast<const phase_t*>(phase)) {
            return install(*requested_phase);
        }
    }

    throw std::runtime_error(std::format("m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::install: phase '{}' cannot install requested phase because it is not in its phase chain", name()));
}

template <class phase_t>
std::vector<typename phase_t::installed_t> phase_base_t::install_closure() const {
    std::vector<typename phase_t::installed_t> outputs;

    for (const auto& module_group : m_module.closure_groups()) {
        for (auto* module : module_group) {
            const auto phase = phase_base_t::make(*module, m_build_config);
            outputs.push_back(phase->template install<phase_t>());
        }
    }

    return outputs;
}

template source_phase_t::installed_t phase_base_t::install<source_phase_t>() const;
template interface_phase_t::installed_t phase_base_t::install<interface_phase_t>() const;
template library_phase_t::installed_t phase_base_t::install<library_phase_t>() const;
template binary_phase_t::installed_t phase_base_t::install<binary_phase_t>() const;

} // namespace m03gagbhsujjf63n0w3r2w4q6h_build_phases
