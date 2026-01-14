#include <builder/builder.h>
#include <builder/compiler/cpp_compiler.h>
#include <builder/process/process.h>
#include <builder/shared_library/shared_library.h>

#include <format>
#include <fstream>
#include <iostream>

builder_t::builder_t(const module_graph_t& module_graph, const module_t& module, const path_t& artifacts_dir):
    m_module_graph(module_graph),
    m_module(module),
    m_artifacts_dir(artifacts_dir)
{
}

void builder_t::compile_builder_module_phase(build_phase_t build_phase) const {
    const auto& builder_module = m_module_graph.builder_module();

    std::string make_target;
    switch (build_phase) {
        case build_phase_t::EXPORT_INTERFACE: {
            make_target = "export_interface";
        } break ;
        case build_phase_t::EXPORT_LIBRARIES: {
            make_target = "export_libraries";
        } break ;
        case build_phase_t::IMPORT_LIBRARIES: {
            make_target = "import_libraries";
        } break ;
        default: throw std::runtime_error(std::format("builder_t::compile_builder_module_phase: unknown build_phase {}", static_cast<std::underlying_type_t<build_phase_t>>(build_phase)));
    }

    const auto library_type = library_type_t::SHARED;

    std::vector<process_arg_t> export_command_args;
    export_command_args.push_back(MAKE_PATH);
    export_command_args.push_back("-C");
    export_command_args.push_back(source_dir(builder_module));
    export_command_args.push_back(make_target);
    export_command_args.push_back(std::format("SOURCE_DIR={}", source_dir(builder_module)));
    export_command_args.push_back(std::format("LIBRARY_TYPE={}", library_type_relative_dir(library_type)));
    export_command_args.push_back(std::format("INTERFACE_BUILD_DIR={}", interface_build_dir(builder_module, library_type)));
    export_command_args.push_back(std::format("INTERFACE_INSTALL_DIR={}", interface_install_dir(builder_module, library_type)));
    export_command_args.push_back(std::format("LIBRARIES_BUILD_DIR={}", libraries_build_dir(builder_module, library_type)));
    export_command_args.push_back(std::format("LIBRARIES_INSTALL_DIR={}", libraries_install_dir(builder_module, library_type)));
    export_command_args.push_back(std::format("IMPORT_BUILD_DIR={}", import_build_dir(builder_module)));
    export_command_args.push_back(std::format("IMPORT_INSTALL_DIR={}", import_install_dir(builder_module)));
    export_command_args.push_back(std::format("ARTIFACT_DIR={}", artifact_dir(builder_module)));
    export_command_args.push_back(std::format("ARTIFACT_ALIAS_DIR={}", artifact_alias_dir(builder_module)));

    const int export_command_result = process_t::create_and_wait(export_command_args);
    if (0 < export_command_result) {
        throw std::runtime_error(std::format("builder_t::compile_builder_module_phase: failed to compile builder module phase {}, command exited with code {}", static_cast<std::underlying_type_t<build_phase_t>>(build_phase), export_command_result));
    } else if (export_command_result < 0) {
        throw std::runtime_error(std::format("builder_t::compile_builder_module_phase: failed to compile builder module phase {}, command terminated by signal {}", static_cast<std::underlying_type_t<build_phase_t>>(build_phase), -export_command_result));
    }
}

std::vector<path_t> builder_t::export_interfaces(library_type_t library_type) const {
    std::vector<path_t> exported_interfaces;

    m_module_graph.visit_sccs_topo(m_module_graph.module_scc(m_module), [&](const module_scc_t* scc) {
        for (const auto& module : scc->modules) {
            auto exported_interface = export_interface(*module, library_type);
            exported_interfaces.emplace_back(std::move(exported_interface));
        }
    });

    return exported_interfaces;
}

std::vector<std::vector<path_t>> builder_t::export_libraries(library_type_t library_type) const {
    std::vector<std::vector<path_t>> library_groups;

    m_module_graph.visit_sccs_topo(m_module_graph.module_scc(m_module), [&](const module_scc_t* scc) {
        std::vector<path_t> library_group;

        for (const auto& module : scc->modules) {
            auto libraries = export_libraries(*module, library_type);
            library_group.insert(library_group.end(), std::make_move_iterator(libraries.begin()), std::make_move_iterator(libraries.end()));
        }

        if (!library_group.empty()) {
            library_groups.emplace_back(std::move(library_group));
        }
    });

    return library_groups;
}

void builder_t::import_libraries() const {
    import_libraries(m_module);
}

void builder_t::install_interface(const path_t& interface, const relative_path_t& relative_install_path, library_type_t library_type) const {
    const auto target_path = interface_install_dir(library_type) / relative_path_t(m_module.name()) / relative_install_path;
    const auto target_path_parent = target_path.parent();
    if (!filesystem_t::exists(target_path_parent)) {
        filesystem_t::create_directories(target_path_parent);
    }

    filesystem_t::copy(interface, target_path);
}

void builder_t::install_library(const path_t& library, const relative_path_t& relative_install_path, library_type_t library_type) const {
    const auto target_path = libraries_install_dir(library_type) / relative_install_path;
    const auto target_path_parent = target_path.parent();
    if (!filesystem_t::exists(target_path_parent)) {
        filesystem_t::create_directories(target_path_parent);
    }

    filesystem_t::copy(library, target_path);
}

void builder_t::install_import(const path_t& artifact, const relative_path_t& relative_install_path) const {
    const auto target_path = import_install_dir() / relative_install_path;
    const auto target_path_parent = target_path.parent();
    if (!filesystem_t::exists(target_path_parent)) {
        filesystem_t::create_directories(target_path_parent);
    }

    filesystem_t::copy(artifact, target_path);
}

path_t builder_t::modules_dir() const {
    return m_module_graph.modules_dir();
}

path_t builder_t::artifacts_dir() const {
    return m_artifacts_dir;
}

path_t builder_t::source_dir() const {
    return source_dir(m_module);
}

path_t builder_t::artifact_dir() const {
    return artifact_dir(m_module);
}

path_t builder_t::artifact_alias_dir() const {
    return artifact_alias_dir(m_module);
}

path_t builder_t::builder_source_path() const {
    return builder_source_path(m_module);
}

path_t builder_t::builder_dir() const {
    return builder_dir(m_module);
}

path_t builder_t::builder_build_dir() const {
    return builder_build_dir(m_module);
}

path_t builder_t::builder_install_dir() const {
    return builder_install_dir(m_module);
}

path_t builder_t::builder_install_path() const {
    return builder_install_path(m_module);
}

path_t builder_t::interface_dir() const {
    return interface_dir(m_module);
}

path_t builder_t::interface_build_dir(library_type_t library_type) const {
    return interface_build_dir(m_module, library_type);
}

path_t builder_t::interface_install_dir(library_type_t library_type) const {
    return interface_install_dir(m_module, library_type);
}

path_t builder_t::libraries_dir() const {
    return libraries_dir(m_module);
}

path_t builder_t::libraries_build_dir(library_type_t library_type) const {
    return libraries_build_dir(m_module, library_type);
}

path_t builder_t::libraries_install_dir(library_type_t library_type) const {
    return libraries_install_dir(m_module, library_type);
}

path_t builder_t::import_dir() const {
    return import_dir(m_module);
}

path_t builder_t::import_build_dir() const {
    return import_build_dir(m_module);
}

path_t builder_t::import_install_dir() const {
    return import_install_dir(m_module);
}

path_t builder_t::source_dir(const module_t& module) const {
    return modules_dir() / relative_path_t(module.name());
}

path_t builder_t::artifact_dir(const module_t& module) const {
    return versioned_path_t::make(artifacts_dir(), module.name(), module.version());
}

path_t builder_t::artifact_alias_dir(const module_t& module) const {
    return artifacts_dir() / relative_path_t(module.name()) / relative_path_t("alias");
}

path_t builder_t::builder_source_path(const module_t& module) const {
    return source_dir(module) / relative_path_t(module_t::BUILDER_CPP);
}

path_t builder_t::builder_dir(const module_t& module) const {
    return artifact_dir(module) / relative_path_t("builder");
}

path_t builder_t::builder_build_dir(const module_t& module) const {
    return builder_dir(module) / build_relative_dir();
}

path_t builder_t::builder_install_dir(const module_t& module) const {
    return builder_dir(module) / install_relative_dir();
}

path_t builder_t::builder_install_path(const module_t& module) const {
    return builder_install_dir(module) / relative_path_t("builder.so");
}

path_t builder_t::interface_dir(const module_t& module) const {
    return artifact_dir(module) / relative_path_t("interface");
}

path_t builder_t::interface_build_dir(const module_t& module, library_type_t library_type) const {
    return interface_dir(module) / library_type_relative_dir(library_type) / build_relative_dir();
}

path_t builder_t::interface_install_dir(const module_t& module, library_type_t library_type) const {
    return interface_dir(module) / library_type_relative_dir(library_type) / install_relative_dir();
}

path_t builder_t::libraries_dir(const module_t& module) const {
    return artifact_dir(module) / relative_path_t("libraries");
}

path_t builder_t::libraries_build_dir(const module_t& module, library_type_t library_type) const {
    return libraries_dir(module) / library_type_relative_dir(library_type) / build_relative_dir();
}

path_t builder_t::libraries_install_dir(const module_t& module, library_type_t library_type) const {
    return libraries_dir(module) / library_type_relative_dir(library_type) / install_relative_dir();
}

path_t builder_t::import_dir(const module_t& module) const {
    return artifact_dir(module) / relative_path_t("import");
}

path_t builder_t::import_build_dir(const module_t& module) const {
    return import_dir(module) / build_relative_dir();
}

path_t builder_t::import_install_dir(const module_t& module) const {
    return import_dir(module) / install_relative_dir();
}

path_t builder_t::export_interface(const module_t& module, library_type_t library_type) const {
    const auto module_interface_dir = interface_dir(module);
    const auto module_interface_build_dir = interface_build_dir(module, library_type);
    const auto module_interface_install_dir = interface_install_dir(module, library_type);

    const auto in_progress_marker = module_interface_build_dir / relative_path_t(".in_progress");

    if (filesystem_t::exists(in_progress_marker)) {
        throw std::runtime_error(std::format("builder_t::export_interface: re-entry detected for exporting interface of module '{}'", module.name()));
    }

    if (!filesystem_t::exists(module_interface_install_dir)) {
        try {
            if (!filesystem_t::exists(module_interface_build_dir)) {
                filesystem_t::create_directories(module_interface_build_dir);
            }
            filesystem_t::touch(in_progress_marker);
            filesystem_t::create_directories(module_interface_install_dir);

            if (module == m_module_graph.builder_module()) {
                compile_builder_module_phase(build_phase_t::EXPORT_INTERFACE);
            } else {
                const auto& builder_plugin = build_builder(module);

                shared_library_t builder_shared_library(builder_plugin, shared_library_lifetime_t::PROCESS, symbol_resolution_t::LAZY, symbol_visibility_t::LOCAL);
                typedef void (*builder__export_interface_t)(const builder_t* builder, library_type_t library_type);
                builder__export_interface_t builder__export_interface = builder_shared_library.resolve("builder__export_interface");
                builder_t builder(m_module_graph, module, m_artifacts_dir);
                builder__export_interface(&builder, library_type);
            }

            // NOTE: could remove `module_interface_build_dir` instead at this point as `module_interface_install_dir` marks completion

            filesystem_t::remove(in_progress_marker);
        } catch (...) {
            filesystem_t::remove_all(module_interface_dir);
            throw ;
        }
    }

    return module_interface_install_dir;
}

std::vector<path_t> builder_t::export_libraries(const module_t& module, library_type_t library_type) const {
    const auto module_libraries_dir = libraries_dir(module);
    const auto module_libraries_build_dir = libraries_build_dir(module, library_type);
    const auto module_libraries_install_dir = libraries_install_dir(module, library_type);

    const auto in_progress_marker = module_libraries_build_dir / relative_path_t(".in_progress");

    if (filesystem_t::exists(in_progress_marker)) {
        throw std::runtime_error(std::format("builder_t::export_libraries: re-entry detected for exporting libraries of module '{}'", module.name()));
    }

    if (!filesystem_t::exists(module_libraries_install_dir)) {
        try {
            if (!filesystem_t::exists(module_libraries_build_dir)) {
                filesystem_t::create_directories(module_libraries_build_dir);
            }
            filesystem_t::touch(in_progress_marker);
            filesystem_t::create_directories(module_libraries_install_dir);

            if (module == m_module_graph.builder_module()) {
                compile_builder_module_phase(build_phase_t::EXPORT_LIBRARIES);
            } else {
                const auto& builder_plugin = build_builder(module);

                shared_library_t builder_shared_library(builder_plugin, shared_library_lifetime_t::PROCESS, symbol_resolution_t::LAZY, symbol_visibility_t::LOCAL);
                typedef void (*builder__export_libraries_t)(const builder_t* builder, library_type_t library_type);
                builder__export_libraries_t builder__export_libraries = builder_shared_library.resolve("builder__export_libraries");
                builder_t builder(m_module_graph, module, m_artifacts_dir);
                builder__export_libraries(&builder, library_type);

                const auto module_artifact_alias_dir = artifact_alias_dir(module);
                const auto module_artifact_alias_dir_tmp = module_artifact_alias_dir + "_tmp";
                if (filesystem_t::exists(module_artifact_alias_dir_tmp)) {
                    filesystem_t::remove_all(module_artifact_alias_dir_tmp);
                }

                filesystem_t::create_directory_symlink(artifact_dir(module), module_artifact_alias_dir_tmp);
                filesystem_t::rename_replace(module_artifact_alias_dir_tmp, module_artifact_alias_dir);

                for (const auto& versioned_module : filesystem_t::find(m_artifacts_dir / relative_path_t(module.name()), filesystem_t::is_dir, filesystem_t::descend_none)) {
                    if (versioned_path_t::is_versioned(versioned_module) && versioned_path_t::parse(versioned_module) < module.version()) {
                        filesystem_t::remove_all(versioned_module);
                    }
                }
            }

            // NOTE: could remove `module_libraries_build_dir` instead at this point as `module_libraries_install_dir` marks completion

            filesystem_t::remove(in_progress_marker);
        } catch (...) {
            filesystem_t::remove_all(module_libraries_dir);
            throw ;
        }
    }

    return filesystem_t::find(module_libraries_install_dir, !filesystem_t::is_dir, filesystem_t::descend_all);
}

void builder_t::import_libraries(const module_t& module) const {
    const auto module_import_dir = import_dir(module);
    const auto module_import_build_dir = import_build_dir(module);
    const auto module_import_install_dir = import_install_dir(module);

    const auto in_progress_marker = module_import_build_dir / relative_path_t(".in_progress");

    if (filesystem_t::exists(in_progress_marker)) {
        throw std::runtime_error(std::format("builder_t::import_libraries: re-entry detected for importing libraries of module '{}'", module.name()));
    }

    if (!filesystem_t::exists(module_import_install_dir)) {
        try {
            if (!filesystem_t::exists(module_import_build_dir)) {
                filesystem_t::create_directories(module_import_build_dir);
            }
            filesystem_t::touch(in_progress_marker);
            filesystem_t::create_directories(module_import_install_dir);

            if (module == m_module_graph.builder_module()) {
                compile_builder_module_phase(build_phase_t::IMPORT_LIBRARIES);
            } else {
                const auto& builder_plugin = build_builder(module);

                shared_library_t builder_shared_library(builder_plugin, shared_library_lifetime_t::PROCESS, symbol_resolution_t::LAZY, symbol_visibility_t::LOCAL);
                typedef void (*builder__import_libraries_t)(const builder_t* builder);
                builder__import_libraries_t builder__import_libraries = builder_shared_library.resolve("builder__import_libraries");
                builder_t builder(m_module_graph, module, m_artifacts_dir);
                builder__import_libraries(&builder);
            }

            // NOTE: could remove `module_import_build_dir` instead at this point as `module_import_install_dir` marks completion

            filesystem_t::remove(in_progress_marker);
        } catch (...) {
            filesystem_t::remove_all(module_import_dir);
            throw ;
        }
    }
}

path_t builder_t::build_builder(const module_t& module) const {
    const auto builder = builder_install_path(module);
    if (!filesystem_t::exists(builder)) {
        const auto& builder_module = m_module_graph.builder_module();
        const auto builder_module_library_type = library_type_t::SHARED;
        const auto builder_interface = export_interface(builder_module, builder_module_library_type);
        const auto builder_libraries = export_libraries(builder_module, builder_module_library_type);
        cpp_compiler_t::create_shared_library(
            builder_build_dir(module),
            source_dir(module),
            { builder_interface },
            { builder_source_path(module) },
            {},
            builder_libraries,
            builder
        );
    }

    if (!filesystem_t::exists(builder)) {
        throw std::runtime_error(std::format("builder_t::build_builder: expected builder plugin '{}' to exist but it does not", builder));
    }

    return builder;
}

relative_path_t builder_t::build_relative_dir() const {
    return relative_path_t("build");
}

relative_path_t builder_t::install_relative_dir() const {
    return relative_path_t("install");
}

relative_path_t builder_t::library_type_relative_dir(library_type_t library_type) const {
    switch (library_type) {
        case library_type_t::STATIC: return relative_path_t("static");
        case library_type_t::SHARED: return relative_path_t("shared");
        default: throw std::runtime_error(std::format("builder_t::library_type_relative_dir: unknown library_type {}", static_cast<std::underlying_type_t<library_type_t>>(library_type)));
    }
}
