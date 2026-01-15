#ifndef BUILDER_MODULE_BUILDER_H
# define BUILDER_MODULE_BUILDER_H

# include "module.h"

namespace builder {

# ifdef __cplusplus
#  define BUILDER_EXTERN extern "C"
# else
#  define BUILDER_EXTERN
# endif

enum class library_type_t : uint8_t {
    STATIC,
    SHARED
};

class module_builder_t {
public:
    static const constexpr char* MAKE_PATH = "/usr/bin/make";

    enum class phase_t : uint8_t {
        EXPORT_INTERFACE,
        EXPORT_LIBRARIES,
        IMPORT_LIBRARIES
    };

public:
    module_builder_t(const module_graph_t& module_graph, const module_t& module, const filesystem::path_t& artifacts_dir);

    void compile_builder_module_phase(phase_t phase) const;

    std::vector<filesystem::path_t> export_interfaces(library_type_t library_type) const;
    std::vector<std::vector<filesystem::path_t>> export_libraries(library_type_t library_type) const;
    void import_libraries() const;

    void install_interface(const filesystem::path_t& interface, const filesystem::relative_path_t& relative_install_path, library_type_t library_type) const;
    void install_library(const filesystem::path_t& library, const filesystem::relative_path_t& relative_install_path, library_type_t library_type) const;
    void install_import(const filesystem::path_t& artifact, const filesystem::relative_path_t& relative_install_path) const;

    filesystem::path_t modules_dir() const;
    filesystem::path_t artifacts_dir() const;

    filesystem::path_t source_dir() const;
    filesystem::path_t artifact_dir() const;
    filesystem::path_t artifact_alias_dir() const;

    filesystem::path_t builder_source_path() const;
    filesystem::path_t builder_dir() const;
    filesystem::path_t builder_build_dir() const;
    filesystem::path_t builder_install_dir() const;
    filesystem::path_t builder_install_path() const;

    filesystem::path_t interface_dir() const;
    filesystem::path_t interface_build_dir(library_type_t library_type) const;
    filesystem::path_t interface_install_dir(library_type_t library_type) const;

    filesystem::path_t libraries_dir() const;
    filesystem::path_t libraries_build_dir(library_type_t library_type) const;
    filesystem::path_t libraries_install_dir(library_type_t library_type) const;

    filesystem::path_t import_dir() const;
    filesystem::path_t import_build_dir() const;
    filesystem::path_t import_install_dir() const;

private:
    void run_export_interface(const module_t& module, library_type_t library_type) const;
    void run_export_libraries(const module_t& module, library_type_t library_type) const;
    void run_import_libraries(const module_t& module) const;

    filesystem::path_t source_dir(const module_t& module) const;
    filesystem::path_t artifact_dir(const module_t& module) const;
    filesystem::path_t artifact_alias_dir(const module_t& module) const;

    filesystem::path_t builder_source_path(const module_t& module) const;
    filesystem::path_t builder_dir(const module_t& module) const;
    filesystem::path_t builder_build_dir(const module_t& module) const;
    filesystem::path_t builder_install_dir(const module_t& module) const;
    filesystem::path_t builder_install_path(const module_t& module) const;

    filesystem::path_t interface_dir(const module_t& module) const;
    filesystem::path_t interface_build_dir(const module_t& module, library_type_t library_type) const;
    filesystem::path_t interface_install_dir(const module_t& module, library_type_t library_type) const;

    filesystem::path_t libraries_dir(const module_t& module) const;
    filesystem::path_t libraries_build_dir(const module_t& module, library_type_t library_type) const;
    filesystem::path_t libraries_install_dir(const module_t& module, library_type_t library_type) const;

    filesystem::path_t import_dir(const module_t& module) const;
    filesystem::path_t import_build_dir(const module_t& module) const;
    filesystem::path_t import_install_dir(const module_t& module) const;

private:
    filesystem::path_t build_builder(const module_t& module) const;
    filesystem::relative_path_t build_relative_dir() const;
    filesystem::relative_path_t install_relative_dir() const;
    filesystem::relative_path_t library_type_relative_dir(library_type_t library_type) const;

private:
    const module_graph_t& m_module_graph;
    const module_t& m_module;
    filesystem::path_t m_artifacts_dir;
};

BUILDER_EXTERN void module_builder__export_interface(const module_builder_t* module_builder, library_type_t library_type);
BUILDER_EXTERN void module_builder__export_libraries(const module_builder_t* module_builder, library_type_t library_type);
BUILDER_EXTERN void module_builder__import_libraries(const module_builder_t* module_builder);

} // namespace builder

#endif // BUILDER_MODULE_BUILDER_H
