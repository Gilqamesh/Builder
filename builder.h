#ifndef BUILDER_PROJECT_BUILDER_BUILDER_H
# define BUILDER_PROJECT_BUILDER_BUILDER_H

# include <builder/module_graph.h>

# ifdef __cplusplus
#  define BUILDER_EXTERN extern "C"
# else
#  define BUILDER_EXTERN
# endif

enum class library_type_t : uint8_t {
    STATIC,
    SHARED
};

enum class build_phase_t : uint8_t {
    EXPORT_INTERFACE,
    EXPORT_LIBRARIES,
    IMPORT_LIBRARIES
};

class builder_t {
public:
    builder_t(const module_graph_t& module_graph, const module_t& module, const path_t& artifacts_dir);

    void compile_builder_module_phase(build_phase_t build_phase) const;

    std::vector<path_t> export_interfaces(library_type_t library_type) const;
    std::vector<std::vector<path_t>> export_libraries(library_type_t library_type) const;
    void import_libraries() const;

    void install_interface(const path_t& interface, library_type_t library_type) const;
    void install_library(const path_t& library, library_type_t library_type) const;
    void install_import(const path_t& artifact) const;

    path_t modules_dir() const;
    path_t artifacts_dir() const;

    path_t source_dir() const;
    path_t artifact_dir() const;

    path_t builder_source_path() const;
    path_t builder_dir() const;
    path_t builder_build_dir() const;
    path_t builder_install_dir() const;
    path_t builder_install_path() const;

    path_t interface_dir() const;
    path_t interface_build_dir(library_type_t library_type) const;
    path_t interface_install_dir(library_type_t library_type) const;

    path_t libraries_dir() const;
    path_t libraries_build_dir(library_type_t library_type) const;
    path_t libraries_install_dir(library_type_t library_type) const;

    path_t import_dir() const;
    path_t import_build_dir() const;
    path_t import_install_dir() const;

private:
    path_t export_interface(const module_t& module, library_type_t library_type) const;
    std::vector<path_t> export_libraries(const module_t& module, library_type_t library_type) const;
    void import_libraries(const module_t& module) const;

    path_t source_dir(const module_t& module) const;
    path_t artifact_dir(const module_t& module) const;

    path_t builder_source_path(const module_t& module) const;
    path_t builder_dir(const module_t& module) const;
    path_t builder_build_dir(const module_t& module) const;
    path_t builder_install_dir(const module_t& module) const;
    path_t builder_install_path(const module_t& module) const;

    path_t interface_dir(const module_t& module) const;
    path_t interface_build_dir(const module_t& module, library_type_t library_type) const;
    path_t interface_install_dir(const module_t& module, library_type_t library_type) const;

    path_t libraries_dir(const module_t& module) const;
    path_t libraries_build_dir(const module_t& module, library_type_t library_type) const;
    path_t libraries_install_dir(const module_t& module, library_type_t library_type) const;

    path_t import_dir(const module_t& module) const;
    path_t import_build_dir(const module_t& module) const;
    path_t import_install_dir(const module_t& module) const;

private:
    path_t build_builder(const module_t& module) const;
    relative_path_t build_relative_dir() const;
    relative_path_t install_relative_dir() const;
    relative_path_t library_type_relative_dir(library_type_t library_type) const;

private:
    const module_graph_t& m_module_graph;
    const module_t& m_module;
    path_t m_artifacts_dir;
};

BUILDER_EXTERN void builder__export_interface(const builder_t* builder, library_type_t library_type);
BUILDER_EXTERN void builder__export_libraries(const builder_t* builder, library_type_t library_type);
BUILDER_EXTERN void builder__import_libraries(const builder_t* builder);

#endif // BUILDER_PROJECT_BUILDER_BUILDER_H
