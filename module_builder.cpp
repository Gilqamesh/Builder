#include "module_builder.h"
#include "cpp_compiler/cpp_compiler.h"
#include "process/process.h"
#include "shared_library/shared_library.h"

#include <format>
#include <fstream>
#include <iostream>

namespace builder {

module_builder_t::module_builder_t(const module_graph_t& module_graph, const module_t& module, const filesystem::path_t& artifacts_dir):
    m_module_graph(module_graph),
    m_module(module),
    m_artifacts_dir(artifacts_dir)
{
}

void module_builder_t::compile_builder_module_phase(phase_t phase) const {
    const auto& builder_module = m_module_graph.builder_module();

    std::string make_target;
    switch (phase) {
        case phase_t::EXPORT_INTERFACE: {
            make_target = "export_interface";
        } break ;
        case phase_t::EXPORT_LIBRARIES: {
            make_target = "export_libraries";
        } break ;
        case phase_t::IMPORT_LIBRARIES: {
            make_target = "import_libraries";
        } break ;
        default: throw std::runtime_error(std::format("builder::module_builder_t::compile_builder_module_phase: unknown phase {}", static_cast<std::underlying_type_t<phase_t>>(phase)));
    }

    const auto library_type = library_type_t::SHARED;

    const int export_command_result = process::create_and_wait({
        MAKE_PATH,
        "-C",
        source_dir(builder_module),
        make_target,
        std::format("SOURCE_DIR={}", source_dir(builder_module)),
        std::format("LIBRARY_TYPE={}", library_type_relative_dir(library_type)),
        std::format("INTERFACE_BUILD_DIR={}", interface_build_dir(builder_module, library_type)),
        std::format("INTERFACE_INSTALL_DIR={}", interface_install_dir(builder_module, library_type)),
        std::format("LIBRARIES_BUILD_DIR={}", libraries_build_dir(builder_module, library_type)),
        std::format("LIBRARIES_INSTALL_DIR={}", libraries_install_dir(builder_module, library_type)),
        std::format("IMPORT_BUILD_DIR={}", import_build_dir(builder_module)),
        std::format("IMPORT_INSTALL_DIR={}", import_install_dir(builder_module)),
        std::format("ARTIFACT_DIR={}", artifact_dir(builder_module)),
        std::format("ARTIFACT_ALIAS_DIR={}", artifact_alias_dir(builder_module))
    });
    if (0 < export_command_result) {
        throw std::runtime_error(std::format("builder::module_builder_t::compile_builder_module_phase: failed to compile builder module phase {}, command exited with code {}", static_cast<std::underlying_type_t<phase_t>>(phase), export_command_result));
    } else if (export_command_result < 0) {
        throw std::runtime_error(std::format("builder::module_builder_t::compile_builder_module_phase: failed to compile builder module phase {}, command terminated by signal {}", static_cast<std::underlying_type_t<phase_t>>(phase), -export_command_result));
    }
}

std::vector<filesystem::path_t> module_builder_t::export_interfaces(library_type_t library_type) const {
    std::vector<filesystem::path_t> exported_interfaces;

    m_module_graph.visit_sccs_topo(m_module_graph.module_scc(m_module), [&](const module_scc_t* scc) {
        for (const auto& module : scc->modules) {
            run_export_interface(*module, library_type);
            if (*module == m_module) {
                exported_interfaces.push_back(interface_install_dir(*module, library_type) / filesystem::relative_path_t(module->name()));
            } else {
                exported_interfaces.push_back(interface_install_dir(*module, library_type));
            }
        }
    });

    return exported_interfaces;
}

std::vector<std::vector<filesystem::path_t>> module_builder_t::export_libraries(library_type_t library_type) const {
    std::vector<std::vector<filesystem::path_t>> library_groups;

    m_module_graph.visit_sccs_topo(m_module_graph.module_scc(m_module), [&](const module_scc_t* scc) {
        std::vector<filesystem::path_t> library_group;

        for (const auto& module : scc->modules) {
            run_export_libraries(*module, library_type);
            auto libraries = filesystem::find(libraries_install_dir(*module, library_type), !filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_all);
            library_group.insert(library_group.end(), std::make_move_iterator(libraries.begin()), std::make_move_iterator(libraries.end()));
        }

        if (!library_group.empty()) {
            library_groups.emplace_back(std::move(library_group));
        }
    });

    return library_groups;
}

void module_builder_t::import_libraries() const {
    run_import_libraries(m_module);
}

void module_builder_t::install_interface(const filesystem::path_t& interface, const filesystem::relative_path_t& relative_install_path, library_type_t library_type) const {
    const auto target_path = interface_install_dir(library_type) / relative_install_path;
    const auto target_path_parent = target_path.parent();
    if (!filesystem::exists(target_path_parent)) {
        filesystem::create_directories(target_path_parent);
    }

    filesystem::copy(interface, target_path);
}

void module_builder_t::install_library(const filesystem::path_t& library, const filesystem::relative_path_t& relative_install_path, library_type_t library_type) const {
    const auto target_path = libraries_install_dir(library_type) / relative_install_path;
    const auto target_path_parent = target_path.parent();
    if (!filesystem::exists(target_path_parent)) {
        filesystem::create_directories(target_path_parent);
    }

    filesystem::copy(library, target_path);
}

void module_builder_t::install_import(const filesystem::path_t& artifact, const filesystem::relative_path_t& relative_install_path) const {
    const auto target_path = import_install_dir() / relative_install_path;
    const auto target_path_parent = target_path.parent();
    if (!filesystem::exists(target_path_parent)) {
        filesystem::create_directories(target_path_parent);
    }

    filesystem::copy(artifact, target_path);
}

filesystem::path_t module_builder_t::modules_dir() const {
    return m_module_graph.modules_dir();
}

filesystem::path_t module_builder_t::artifacts_dir() const {
    return m_artifacts_dir;
}

filesystem::path_t module_builder_t::source_dir() const {
    return source_dir(m_module);
}

filesystem::path_t module_builder_t::artifact_dir() const {
    return artifact_dir(m_module);
}

filesystem::path_t module_builder_t::artifact_alias_dir() const {
    return artifact_alias_dir(m_module);
}

filesystem::path_t module_builder_t::builder_source_path() const {
    return builder_source_path(m_module);
}

filesystem::path_t module_builder_t::builder_dir() const {
    return builder_dir(m_module);
}

filesystem::path_t module_builder_t::builder_build_dir() const {
    return builder_build_dir(m_module);
}

filesystem::path_t module_builder_t::builder_install_dir() const {
    return builder_install_dir(m_module);
}

filesystem::path_t module_builder_t::builder_install_path() const {
    return builder_install_path(m_module);
}

filesystem::path_t module_builder_t::interface_dir() const {
    return interface_dir(m_module);
}

filesystem::path_t module_builder_t::interface_build_dir(library_type_t library_type) const {
    return interface_build_dir(m_module, library_type);
}

filesystem::path_t module_builder_t::interface_install_dir(library_type_t library_type) const {
    return interface_install_dir(m_module, library_type);
}

filesystem::path_t module_builder_t::libraries_dir() const {
    return libraries_dir(m_module);
}

filesystem::path_t module_builder_t::libraries_build_dir(library_type_t library_type) const {
    return libraries_build_dir(m_module, library_type);
}

filesystem::path_t module_builder_t::libraries_install_dir(library_type_t library_type) const {
    return libraries_install_dir(m_module, library_type);
}

filesystem::path_t module_builder_t::import_dir() const {
    return import_dir(m_module);
}

filesystem::path_t module_builder_t::import_build_dir() const {
    return import_build_dir(m_module);
}

filesystem::path_t module_builder_t::import_install_dir() const {
    return import_install_dir(m_module);
}

filesystem::path_t module_builder_t::source_dir(const module_t& module) const {
    return modules_dir() / filesystem::relative_path_t(module.name());
}

filesystem::path_t module_builder_t::artifact_dir(const module_t& module) const {
    return versioned_path_t::make(artifacts_dir(), module.name(), module.version());
}

filesystem::path_t module_builder_t::artifact_alias_dir(const module_t& module) const {
    return artifacts_dir() / filesystem::relative_path_t(module.name()) / filesystem::relative_path_t("alias");
}

filesystem::path_t module_builder_t::builder_source_path(const module_t& module) const {
    return source_dir(module) / filesystem::relative_path_t(module_t::BUILDER_CPP);
}

filesystem::path_t module_builder_t::builder_dir(const module_t& module) const {
    return artifact_dir(module) / filesystem::relative_path_t("builder");
}

filesystem::path_t module_builder_t::builder_build_dir(const module_t& module) const {
    return builder_dir(module) / build_relative_dir();
}

filesystem::path_t module_builder_t::builder_install_dir(const module_t& module) const {
    return builder_dir(module) / install_relative_dir();
}

filesystem::path_t module_builder_t::builder_install_path(const module_t& module) const {
    return builder_install_dir(module) / filesystem::relative_path_t("builder.so");
}

filesystem::path_t module_builder_t::interface_dir(const module_t& module) const {
    return artifact_dir(module) / filesystem::relative_path_t("interface");
}

filesystem::path_t module_builder_t::interface_build_dir(const module_t& module, library_type_t library_type) const {
    return interface_dir(module) / library_type_relative_dir(library_type) / build_relative_dir();
}

filesystem::path_t module_builder_t::interface_install_dir(const module_t& module, library_type_t library_type) const {
    return interface_dir(module) / library_type_relative_dir(library_type) / install_relative_dir() / filesystem::relative_path_t(module.name());
}

filesystem::path_t module_builder_t::libraries_dir(const module_t& module) const {
    return artifact_dir(module) / filesystem::relative_path_t("libraries");
}

filesystem::path_t module_builder_t::libraries_build_dir(const module_t& module, library_type_t library_type) const {
    return libraries_dir(module) / library_type_relative_dir(library_type) / build_relative_dir();
}

filesystem::path_t module_builder_t::libraries_install_dir(const module_t& module, library_type_t library_type) const {
    return libraries_dir(module) / library_type_relative_dir(library_type) / install_relative_dir();
}

filesystem::path_t module_builder_t::import_dir(const module_t& module) const {
    return artifact_dir(module) / filesystem::relative_path_t("import");
}

filesystem::path_t module_builder_t::import_build_dir(const module_t& module) const {
    return import_dir(module) / build_relative_dir();
}

filesystem::path_t module_builder_t::import_install_dir(const module_t& module) const {
    return import_dir(module) / install_relative_dir();
}

void module_builder_t::run_export_interface(const module_t& module, library_type_t library_type) const {
    const auto module_interface_dir = interface_dir(module);
    const auto module_interface_build_dir = interface_build_dir(module, library_type);
    const auto module_interface_install_dir = interface_install_dir(module, library_type);

    const auto in_progress_marker = module_interface_build_dir / filesystem::relative_path_t(".in_progress");

    if (filesystem::exists(in_progress_marker)) {
        throw std::runtime_error(std::format("builder::module_builder_t::run_export_interface: re-entry detected for exporting interface of module '{}'", module.name()));
    }

    if (!filesystem::exists(module_interface_install_dir)) {
        try {
            if (!filesystem::exists(module_interface_build_dir)) {
                filesystem::create_directories(module_interface_build_dir);
            }
            filesystem::touch(in_progress_marker);
            filesystem::create_directories(module_interface_install_dir);

            if (module == m_module_graph.builder_module()) {
                compile_builder_module_phase(phase_t::EXPORT_INTERFACE);
            } else {
                const auto& builder_plugin = build_builder(module);

                shared_library::loader_t loader(builder_plugin, shared_library::lifetime_t::PROCESS, shared_library::symbol_resolution_t::LAZY, shared_library::symbol_visibility_t::LOCAL);
                typedef void (*module_builder__export_interface_t)(const module_builder_t* module_builder, library_type_t library_type);
                module_builder__export_interface_t module_builder__export_interface = loader.resolve("module_builder__export_interface");
                module_builder_t module_builder(m_module_graph, module, m_artifacts_dir);
                module_builder__export_interface(&module_builder, library_type);
            }

            // NOTE: could remove `module_interface_build_dir` instead at this point as `module_interface_install_dir` marks completion

            filesystem::remove(in_progress_marker);
        } catch (...) {
            filesystem::remove_all(module_interface_dir);
            throw ;
        }
    }
}

void module_builder_t::run_export_libraries(const module_t& module, library_type_t library_type) const {
    const auto module_libraries_dir = libraries_dir(module);
    const auto module_libraries_build_dir = libraries_build_dir(module, library_type);
    const auto module_libraries_install_dir = libraries_install_dir(module, library_type);

    const auto in_progress_marker = module_libraries_build_dir / filesystem::relative_path_t(".in_progress");

    if (filesystem::exists(in_progress_marker)) {
        throw std::runtime_error(std::format("builder::module_builder_t::export_libraries: re-entry detected for exporting libraries of module '{}'", module.name()));
    }

    if (!filesystem::exists(module_libraries_install_dir)) {
        try {
            if (!filesystem::exists(module_libraries_build_dir)) {
                filesystem::create_directories(module_libraries_build_dir);
            }
            filesystem::touch(in_progress_marker);
            filesystem::create_directories(module_libraries_install_dir);

            if (module == m_module_graph.builder_module()) {
                compile_builder_module_phase(phase_t::EXPORT_LIBRARIES);
            } else {
                const auto& builder_plugin = build_builder(module);

                shared_library::loader_t loader(builder_plugin, shared_library::lifetime_t::PROCESS, shared_library::symbol_resolution_t::LAZY, shared_library::symbol_visibility_t::LOCAL);
                typedef void (*module_builder__export_libraries_t)(const module_builder_t* module_builder, library_type_t library_type);
                module_builder__export_libraries_t module_builder__export_libraries = loader.resolve("module_builder__export_libraries");
                module_builder_t module_builder(m_module_graph, module, m_artifacts_dir);
                module_builder__export_libraries(&module_builder, library_type);

                const auto module_artifact_alias_dir = artifact_alias_dir(module);
                const auto module_artifact_alias_dir_tmp = module_artifact_alias_dir + "_tmp";
                if (filesystem::exists(module_artifact_alias_dir_tmp)) {
                    filesystem::remove_all(module_artifact_alias_dir_tmp);
                }

                filesystem::create_directory_symlink(artifact_dir(module), module_artifact_alias_dir_tmp);
                filesystem::rename_replace(module_artifact_alias_dir_tmp, module_artifact_alias_dir);

                for (const auto& versioned_module : filesystem::find(m_artifacts_dir / filesystem::relative_path_t(module.name()), filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_none)) {
                    if (versioned_path_t::is_versioned(versioned_module) && versioned_path_t::parse(versioned_module) < module.version()) {
                        filesystem::remove_all(versioned_module);
                    }
                }
            }

            // NOTE: could remove `module_libraries_build_dir` instead at this point as `module_libraries_install_dir` marks completion

            filesystem::remove(in_progress_marker);
        } catch (...) {
            filesystem::remove_all(module_libraries_dir);
            throw ;
        }
    }
}

void module_builder_t::run_import_libraries(const module_t& module) const {
    const auto module_import_dir = import_dir(module);
    const auto module_import_build_dir = import_build_dir(module);
    const auto module_import_install_dir = import_install_dir(module);

    const auto in_progress_marker = module_import_build_dir / filesystem::relative_path_t(".in_progress");

    if (filesystem::exists(in_progress_marker)) {
        throw std::runtime_error(std::format("builder::module_builder_t::import_libraries: re-entry detected for importing libraries of module '{}'", module.name()));
    }

    if (!filesystem::exists(module_import_install_dir)) {
        try {
            if (!filesystem::exists(module_import_build_dir)) {
                filesystem::create_directories(module_import_build_dir);
            }
            filesystem::touch(in_progress_marker);
            filesystem::create_directories(module_import_install_dir);

            if (module == m_module_graph.builder_module()) {
                compile_builder_module_phase(phase_t::IMPORT_LIBRARIES);
            } else {
                const auto& builder_plugin = build_builder(module);

                shared_library::loader_t loader(builder_plugin, shared_library::lifetime_t::PROCESS, shared_library::symbol_resolution_t::LAZY, shared_library::symbol_visibility_t::LOCAL);
                typedef void (*module_builder__import_libraries_t)(const module_builder_t* module_builder);
                module_builder__import_libraries_t module_builder__import_libraries = loader.resolve("module_builder__import_libraries");
                module_builder_t module_builder(m_module_graph, module, m_artifacts_dir);
                module_builder__import_libraries(&module_builder);
            }

            // NOTE: could remove `module_import_build_dir` instead at this point as `module_import_install_dir` marks completion

            filesystem::remove(in_progress_marker);
        } catch (...) {
            filesystem::remove_all(module_import_dir);
            throw ;
        }
    }
}

filesystem::path_t module_builder_t::build_builder(const module_t& module) const {
    const auto builder = builder_install_path(module);
    if (!filesystem::exists(builder)) {
        const auto& builder_module = m_module_graph.builder_module();
        const auto builder_module_library_type = library_type_t::SHARED;
        run_export_interface(builder_module, builder_module_library_type);
        const auto builder_interface = interface_install_dir(builder_module, builder_module_library_type);
        run_export_libraries(builder_module, builder_module_library_type);
        const auto builder_libraries = filesystem::find(libraries_install_dir(builder_module, builder_module_library_type), !filesystem::find_include_predicate_t::is_dir, filesystem::find_descend_predicate_t::descend_all);
        cpp_compiler::create_shared_library(
            builder_build_dir(module),
            source_dir(module),
            { builder_interface },
            { builder_source_path(module) },
            {},
            builder_libraries,
            builder
        );
    }

    if (!filesystem::exists(builder)) {
        throw std::runtime_error(std::format("builder::module_builder_t::build_builder: expected builder plugin '{}' to exist but it does not", builder));
    }

    return builder;
}

filesystem::relative_path_t module_builder_t::build_relative_dir() const {
    return filesystem::relative_path_t("build");
}

filesystem::relative_path_t module_builder_t::install_relative_dir() const {
    return filesystem::relative_path_t("install");
}

filesystem::relative_path_t module_builder_t::library_type_relative_dir(library_type_t library_type) const {
    switch (library_type) {
        case library_type_t::STATIC: return filesystem::relative_path_t("static");
        case library_type_t::SHARED: return filesystem::relative_path_t("shared");
        default: throw std::runtime_error(std::format("builder::module_builder_t::library_type_relative_dir: unknown library_type {}", static_cast<std::underlying_type_t<library_type_t>>(library_type)));
    }
}

} // namespace builder
