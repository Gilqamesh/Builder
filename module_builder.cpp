#include "module_builder.h"

#include "compiler.h"
#include "shared_library.h"

#include <format>
#include <functional>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>

namespace kernel {

namespace cpp_builder {

namespace builder {

namespace {

inline const constexpr char* ARTIFACTS_ROOT_DIR = ".cpp_builder_artifacts";

std::string module_display_name(const graph::module_t& module) {
    return std::format("{}/{}", module.workspace->workspace_relative_path_to_workspace_ecosystem.string(), module.module_relative_path_to_workspace.string());
}

const char* phase_name(module_builder_t::phase_t phase) {
    switch (phase) {
        case module_builder_t::phase_t::EXPORT_INTERFACE: return "export_interface";
        case module_builder_t::phase_t::EXPORT_LIBRARIES: return "export_libraries";
        case module_builder_t::phase_t::IMPORT_LIBRARIES: return "import_libraries";
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::phase_name: unknown phase {}", static_cast<std::underlying_type_t<module_builder_t::phase_t>>(phase)));
    }
}

const char* phase_symbol_name(module_builder_t::phase_t phase) {
    switch (phase) {
        case module_builder_t::phase_t::EXPORT_INTERFACE: return "module_builder__export_interface";
        case module_builder_t::phase_t::EXPORT_LIBRARIES: return "module_builder__export_libraries";
        case module_builder_t::phase_t::IMPORT_LIBRARIES: return "module_builder__import_libraries";
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::phase_symbol_name: unknown phase {}", static_cast<std::underlying_type_t<module_builder_t::phase_t>>(phase)));
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

std::vector<filesystem::path_t> kernel_library_source_files(const filesystem::path_t& source_dir) {
    return {
        source_dir / filesystem::relative_path_t("filesystem.cpp"),
        source_dir / filesystem::relative_path_t("process.cpp"),
        source_dir / filesystem::relative_path_t("compiler.cpp"),
        source_dir / filesystem::relative_path_t("shared_library.cpp"),
        source_dir / filesystem::relative_path_t("graph.cpp"),
        source_dir / filesystem::relative_path_t("module_builder.cpp")
    };
}

void visit_sccs_topo_impl(const graph::module_scc_t* scc, const std::function<void(const graph::module_scc_t*)>& f, std::unordered_set<const graph::module_scc_t*>& visited) {
    if (!visited.insert(scc).second) {
        return ;
    }

    for (const auto* dependency : scc->dependencies) {
        visit_sccs_topo_impl(dependency, f, visited);
    }

    f(scc);
}

void visit_sccs_topo(const graph::module_scc_t* scc, const std::function<void(const graph::module_scc_t*)>& f) {
    std::unordered_set<const graph::module_scc_t*> visited;
    visit_sccs_topo_impl(scc, f, visited);
}

} // namespace

module_builder_t::module_builder_t(graph::workspace_ecosystem_t& workspace_ecosystem, graph::module_t& module, const filesystem::path_t& artifacts_dir):
    m_workspace_ecosystem(workspace_ecosystem),
    m_module(module),
    m_artifacts_dir(artifacts_dir)
{
}

std::vector<filesystem::path_t> module_builder_t::export_interfaces(library_type_t library_type) const {
    std::vector<filesystem::path_t> exported_interfaces;

    visit_sccs_topo(m_module.module_scc, [&](const graph::module_scc_t* scc) {
        for (auto* module : scc->modules) {
            run_phase(*module, phase_t::EXPORT_INTERFACE, library_type);
            exported_interfaces.push_back(interface_install_dir(*module, library_type));
        }
    });

    return exported_interfaces;
}

std::vector<std::vector<filesystem::path_t>> module_builder_t::export_libraries(library_type_t library_type) const {
    std::vector<std::vector<filesystem::path_t>> library_groups;

    visit_sccs_topo(m_module.module_scc, [&](const graph::module_scc_t* scc) {
        std::vector<filesystem::path_t> library_group;

        for (auto* module : scc->modules) {
            run_phase(*module, phase_t::EXPORT_LIBRARIES, library_type);
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
    run_phase(m_module, phase_t::IMPORT_LIBRARIES, library_type_t::SHARED);
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

filesystem::path_t module_builder_t::workspace_ecosystem_dir() const {
    return m_workspace_ecosystem.absolute_path_to_workspace_directory;
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

filesystem::path_t module_builder_t::source_dir(const graph::module_t& module) const {
    return module.source_dir();
}

filesystem::path_t module_builder_t::artifact_base_dir(const graph::module_t& module) const {
    return m_artifacts_dir / filesystem::relative_path_t(ARTIFACTS_ROOT_DIR) / module.workspace->workspace_relative_path_to_workspace_ecosystem / module.module_relative_path_to_workspace;
}

filesystem::path_t module_builder_t::artifact_dir(const graph::module_t& module) const {
    const auto versioned_dir_name = std::format("{}@{}", module.module_relative_path_to_workspace.to_native_path().filename().string(), module.version.value);
    return artifact_base_dir(module) / filesystem::relative_path_t(versioned_dir_name);
}

filesystem::path_t module_builder_t::artifact_alias_dir(const graph::module_t& module) const {
    return artifact_base_dir(module) / filesystem::relative_path_t("alias");
}

filesystem::path_t module_builder_t::builder_source_path(const graph::module_t& module) const {
    return module.builder_path();
}

filesystem::path_t module_builder_t::builder_dir(const graph::module_t& module) const {
    return artifact_dir(module) / filesystem::relative_path_t("builder");
}

filesystem::path_t module_builder_t::builder_build_dir(const graph::module_t& module) const {
    return builder_dir(module) / build_relative_dir();
}

filesystem::path_t module_builder_t::builder_install_dir(const graph::module_t& module) const {
    return builder_dir(module) / install_relative_dir();
}

filesystem::path_t module_builder_t::builder_install_path(const graph::module_t& module) const {
    return builder_install_dir(module) / filesystem::relative_path_t("builder.so");
}

filesystem::path_t module_builder_t::interface_dir(const graph::module_t& module) const {
    return artifact_dir(module) / filesystem::relative_path_t("interface");
}

filesystem::path_t module_builder_t::interface_build_dir(const graph::module_t& module, library_type_t library_type) const {
    return interface_dir(module) / library_type_relative_dir(library_type) / build_relative_dir();
}

filesystem::path_t module_builder_t::interface_install_dir(const graph::module_t& module, library_type_t library_type) const {
    return interface_dir(module) / library_type_relative_dir(library_type) / install_relative_dir();
}

filesystem::path_t module_builder_t::libraries_dir(const graph::module_t& module) const {
    return artifact_dir(module) / filesystem::relative_path_t("libraries");
}

filesystem::path_t module_builder_t::libraries_build_dir(const graph::module_t& module, library_type_t library_type) const {
    return libraries_dir(module) / library_type_relative_dir(library_type) / build_relative_dir();
}

filesystem::path_t module_builder_t::libraries_install_dir(const graph::module_t& module, library_type_t library_type) const {
    return libraries_dir(module) / library_type_relative_dir(library_type) / install_relative_dir();
}

filesystem::path_t module_builder_t::import_dir(const graph::module_t& module) const {
    return artifact_dir(module) / filesystem::relative_path_t("import");
}

filesystem::path_t module_builder_t::import_build_dir(const graph::module_t& module) const {
    return import_dir(module) / build_relative_dir();
}

filesystem::path_t module_builder_t::import_install_dir(const graph::module_t& module) const {
    return import_dir(module) / install_relative_dir();
}

filesystem::path_t module_builder_t::phase_dir(const graph::module_t& module, phase_t phase) const {
    switch (phase) {
        case phase_t::EXPORT_INTERFACE: return interface_dir(module);
        case phase_t::EXPORT_LIBRARIES: return libraries_dir(module);
        case phase_t::IMPORT_LIBRARIES: return import_dir(module);
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::module_builder_t::phase_dir: unknown phase {}", static_cast<std::underlying_type_t<phase_t>>(phase)));
    }
}

filesystem::path_t module_builder_t::phase_build_dir(const graph::module_t& module, phase_t phase, library_type_t library_type) const {
    switch (phase) {
        case phase_t::EXPORT_INTERFACE: return interface_build_dir(module, library_type);
        case phase_t::EXPORT_LIBRARIES: return libraries_build_dir(module, library_type);
        case phase_t::IMPORT_LIBRARIES: return import_build_dir(module);
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::module_builder_t::phase_build_dir: unknown phase {}", static_cast<std::underlying_type_t<phase_t>>(phase)));
    }
}

filesystem::path_t module_builder_t::phase_install_dir(const graph::module_t& module, phase_t phase, library_type_t library_type) const {
    switch (phase) {
        case phase_t::EXPORT_INTERFACE: return interface_install_dir(module, library_type);
        case phase_t::EXPORT_LIBRARIES: return libraries_install_dir(module, library_type);
        case phase_t::IMPORT_LIBRARIES: return import_install_dir(module);
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::module_builder_t::phase_install_dir: unknown phase {}", static_cast<std::underlying_type_t<phase_t>>(phase)));
    }
}

void module_builder_t::run_phase(graph::module_t& module, phase_t phase, library_type_t library_type) const {
    const auto root_dir = phase_dir(module, phase);
    const auto build_dir = phase_build_dir(module, phase, library_type);
    const auto install_dir = phase_install_dir(module, phase, library_type);
    const auto marker_path = [&](phase_t marker_phase, std::string_view state) {
        return phase_build_dir(module, marker_phase, library_type) / filesystem::relative_path_t(std::format("{}.{}", phase_name(marker_phase), state));
    };
    const auto started_marker = marker_path(phase, "started");
    const auto complete_marker = marker_path(phase, "complete");

    if (filesystem::exists(complete_marker)) {
        return ;
    }

    if (filesystem::exists(started_marker)) {
        throw std::runtime_error(std::format("kernel::cpp_builder::builder::module_builder_t::run_phase: re-entry detected for phase '{}' of module '{}'", phase_name(phase), module_display_name(module)));
    }

    switch (phase) {
        case phase_t::EXPORT_INTERFACE: {
        } break ;
        case phase_t::EXPORT_LIBRARIES: {
            run_phase(module, phase_t::EXPORT_INTERFACE, library_type);
        } break ;
        case phase_t::IMPORT_LIBRARIES: {
            run_phase(module, phase_t::EXPORT_LIBRARIES, library_type);
        } break ;
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::module_builder_t::run_phase: unknown phase {}", static_cast<std::underlying_type_t<phase_t>>(phase)));
    }

    if (filesystem::exists(root_dir)) {
        filesystem::remove_all(root_dir);
    }

    try {
        if (!filesystem::exists(build_dir)) {
            filesystem::create_directories(build_dir);
        }
        filesystem::touch(started_marker);
        filesystem::create_directories(install_dir);

        if (&module == m_workspace_ecosystem.this_module) {
            run_kernel_phase(module, phase, library_type);
        } else {
            run_module_producer_phase(module, phase, library_type);
        }

        if (phase == phase_t::EXPORT_LIBRARIES) {
            const auto alias_dir = artifact_alias_dir(module);
            const auto alias_tmp_dir = alias_dir + "_tmp";
            if (filesystem::exists(alias_tmp_dir)) {
                filesystem::remove_all(alias_tmp_dir);
            }
            filesystem::create_directory_symlink(artifact_dir(module), alias_tmp_dir);
            filesystem::rename_replace(alias_tmp_dir, alias_dir);
        }

        filesystem::touch(complete_marker);
        filesystem::remove(started_marker);
    } catch (...) {
        filesystem::remove_all(root_dir);
        throw ;
    }
}

void module_builder_t::run_module_producer_phase(graph::module_t& module, phase_t phase, library_type_t library_type) const {
    const auto builder_plugin = build_builder(module);
    shared_library::loader_t loader(builder_plugin, shared_library::lifetime_t::PROCESS, shared_library::symbol_resolution_t::LAZY, shared_library::symbol_visibility_t::LOCAL);
    module_builder_t module_builder(m_workspace_ecosystem, module, m_artifacts_dir);

    switch (phase) {
        case phase_t::EXPORT_INTERFACE:
        case phase_t::EXPORT_LIBRARIES: {
            using fn_t = void (*)(const module_builder_t*, library_type_t);
            fn_t fn = loader.resolve(phase_symbol_name(phase));
            fn(&module_builder, library_type);
        } break ;
        case phase_t::IMPORT_LIBRARIES: {
            using fn_t = void (*)(const module_builder_t*);
            fn_t fn = loader.resolve(phase_symbol_name(phase));
            fn(&module_builder);
        } break ;
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::module_builder_t::run_module_producer_phase: unknown phase {}", static_cast<std::underlying_type_t<phase_t>>(phase)));
    }
}

void module_builder_t::run_kernel_phase(graph::module_t& module, phase_t phase, library_type_t library_type) const {
    switch (phase) {
        case phase_t::EXPORT_INTERFACE: {
            run_kernel_export_interface(module, library_type);
        } break ;
        case phase_t::EXPORT_LIBRARIES: {
            run_kernel_export_libraries(module, library_type);
        } break ;
        case phase_t::IMPORT_LIBRARIES: {
            run_kernel_import_libraries(module);
        } break ;
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::module_builder_t::run_kernel_phase: unknown phase {}", static_cast<std::underlying_type_t<phase_t>>(phase)));
    }
}

void module_builder_t::run_kernel_export_interface(graph::module_t& module, library_type_t library_type) const {
    for (const auto& interface : filesystem::find(source_dir(module), filesystem::find_include_predicate_t::h_file || filesystem::find_include_predicate_t::hpp_file, filesystem::find_descend_predicate_t::descend_all)) {
        const auto relative_interface = source_dir(module).relative(interface);
        const auto target_path = interface_install_dir(module, library_type) / relative_interface;
        const auto target_path_parent = target_path.parent();
        if (!filesystem::exists(target_path_parent)) {
            filesystem::create_directories(target_path_parent);
        }
        filesystem::copy(interface, target_path);
    }
}

void module_builder_t::run_kernel_export_libraries(graph::module_t& module, library_type_t library_type) const {
    const auto module_source_dir = source_dir(module);
    const auto library_name = [&]() {
        switch (library_type) {
            case library_type_t::STATIC: return filesystem::relative_path_t("libcpp_builder.a");
            case library_type_t::SHARED: return filesystem::relative_path_t("libcpp_builder.so");
            default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::module_builder_t::run_kernel_export_libraries: unknown library_type {}", static_cast<std::underlying_type_t<library_type_t>>(library_type)));
        }
    }();

    const auto library_path = libraries_install_dir(module, library_type) / library_name;

    switch (library_type) {
        case library_type_t::STATIC: {
            compiler::create_static_library(
                libraries_build_dir(module, library_type),
                module_source_dir,
                { interface_install_dir(module, library_type) },
                kernel_library_source_files(module_source_dir),
                compiler_path_defines(),
                library_path
            );
        } break ;
        case library_type_t::SHARED: {
            compiler::create_shared_library(
                libraries_build_dir(module, library_type),
                module_source_dir,
                { interface_install_dir(module, library_type) },
                kernel_library_source_files(module_source_dir),
                compiler_path_defines(),
                {},
                library_path
            );
        } break ;
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::module_builder_t::run_kernel_export_libraries: unknown library_type {}", static_cast<std::underlying_type_t<library_type_t>>(library_type)));
    }
}

void module_builder_t::run_kernel_import_libraries(graph::module_t& module) const {
    const auto module_source_dir = source_dir(module);
    const auto library_path = libraries_install_dir(module, library_type_t::SHARED) / filesystem::relative_path_t("libcpp_builder.so");

    compiler::create_binary(
        import_build_dir(module),
        module_source_dir,
        { interface_install_dir(module, library_type_t::SHARED) },
        { module_source_dir / filesystem::relative_path_t("cli.cpp") },
        compiler_path_defines(),
        { { library_path } },
        true,
        import_install_dir(module) / filesystem::relative_path_t("cli")
    );
}

filesystem::path_t module_builder_t::build_builder(graph::module_t& module) const {
    const auto builder_plugin = builder_install_path(module);
    if (filesystem::exists(builder_plugin)) {
        return builder_plugin;
    }

    std::vector<filesystem::path_t> include_dirs;
    std::vector<filesystem::path_t> libraries;

    for (auto* dependency : module.module_builder->dependencies) {
        module_builder_t dependency_builder(m_workspace_ecosystem, *dependency, m_artifacts_dir);
        auto dependency_interfaces = dependency_builder.export_interfaces(library_type_t::SHARED);
        include_dirs.insert(include_dirs.end(), std::make_move_iterator(dependency_interfaces.begin()), std::make_move_iterator(dependency_interfaces.end()));

        auto dependency_library_groups = dependency_builder.export_libraries(library_type_t::SHARED);
        for (auto& dependency_library_group : dependency_library_groups) {
            libraries.insert(libraries.end(), std::make_move_iterator(dependency_library_group.begin()), std::make_move_iterator(dependency_library_group.end()));
        }
    }

    compiler::create_shared_library(
        builder_build_dir(module),
        source_dir(module),
        include_dirs,
        { builder_source_path(module) },
        compiler_path_defines(),
        libraries,
        builder_plugin
    );

    if (!filesystem::exists(builder_plugin)) {
        throw std::runtime_error(std::format("kernel::cpp_builder::builder::module_builder_t::build_builder: expected builder plugin '{}' to exist but it does not", builder_plugin));
    }

    return builder_plugin;
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
        default: throw std::runtime_error(std::format("kernel::cpp_builder::builder::module_builder_t::library_type_relative_dir: unknown library_type {}", static_cast<std::underlying_type_t<library_type_t>>(library_type)));
    }
}

} // namespace builder

} // namespace cpp_builder

} // namespace kernel
