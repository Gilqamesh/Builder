#ifndef KERNEL_CPP_BUILDER_MODULE_BUILDER_H
# define KERNEL_CPP_BUILDER_MODULE_BUILDER_H

# include "graph.h"

# include <cstdint>
# include <string_view>
# include <vector>

namespace kernel {

namespace cpp_builder {

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

class module_builder_t;

struct iphase_t {
    virtual ~iphase_t() = default;

    virtual std::string_view name() const = 0;
    virtual const iphase_t* predecessor() const = 0;
    virtual graph::module_t& module() const = 0;

    virtual filesystem::path_t source_dir() const = 0;
    virtual filesystem::path_t dir() const = 0;
    virtual filesystem::path_t build_dir() const = 0;
    virtual filesystem::path_t install_dir() const = 0;

    virtual void run() const = 0;
};

struct export_interface_output_t {
    std::vector<filesystem::path_t> interfaces;
};

struct export_libraries_output_t {
    std::vector<std::vector<filesystem::path_t>> library_groups;
};

struct import_libraries_output_t {
};

class phase_base_t : public iphase_t {
public:
    phase_base_t(std::string_view name, module_builder_t& module_builder, graph::module_t& module, const iphase_t* predecessor);

    std::string_view name() const override;
    const iphase_t* predecessor() const override;
    graph::module_t& module() const override;

    filesystem::path_t source_dir() const override;

protected:
    module_builder_t& module_builder() const;

private:
    std::string_view m_name;
    module_builder_t& m_module_builder;
    graph::module_t& m_module;
    const iphase_t* m_predecessor;
};

struct export_interface_phase_t : phase_base_t {
    using output_t = export_interface_output_t;

    export_interface_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor = nullptr);

    filesystem::path_t dir() const override;
    filesystem::path_t build_dir() const override;
    filesystem::path_t install_dir() const override;

    void run() const override;

    const library_type_t library_type;
};

struct export_libraries_phase_t : phase_base_t {
    using output_t = export_libraries_output_t;

    export_libraries_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor = nullptr);
    filesystem::path_t dir() const override;
    filesystem::path_t build_dir() const override;
    filesystem::path_t install_dir() const override;

    void run() const override;

    const library_type_t library_type;
};

struct import_libraries_phase_t : phase_base_t {
    using output_t = import_libraries_output_t;

    import_libraries_phase_t(module_builder_t& module_builder, graph::module_t& module, const iphase_t* predecessor = nullptr);
    filesystem::path_t dir() const override;
    filesystem::path_t build_dir() const override;
    filesystem::path_t install_dir() const override;

    void run() const override;
};

class module_builder_t {
public:
    enum class phase_t : uint8_t {
        EXPORT_INTERFACE,
        EXPORT_LIBRARIES,
        IMPORT_LIBRARIES
    };

public:
    module_builder_t(graph::workspace_ecosystem_t& workspace_ecosystem, graph::module_t& module);

    graph::module_t& module() const;

    std::vector<filesystem::path_t> export_interfaces(library_type_t library_type) const;
    std::vector<std::vector<filesystem::path_t>> export_libraries(library_type_t library_type) const;
    void import_libraries() const;

    void install_interface(const filesystem::path_t& interface, const filesystem::relative_path_t& relative_install_path, library_type_t library_type) const;
    void install_library(const filesystem::path_t& library, const filesystem::relative_path_t& relative_install_path, library_type_t library_type) const;
    void install_import(const filesystem::path_t& artifact, const filesystem::relative_path_t& relative_install_path) const;

    filesystem::path_t workspace_ecosystem_dir() const;

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
    void run_phase(graph::module_t& module, phase_t phase, library_type_t library_type) const;
    void run_module_producer_phase(graph::module_t& module, phase_t phase, library_type_t library_type) const;

    void run_kernel_phase(graph::module_t& module, phase_t phase, library_type_t library_type) const;
    void run_kernel_export_interface(graph::module_t& module, library_type_t library_type) const;
    void run_kernel_export_libraries(graph::module_t& module, library_type_t library_type) const;
    void run_kernel_import_libraries(graph::module_t& module) const;

    filesystem::path_t source_dir(const graph::module_t& module) const;
    filesystem::path_t artifact_base_dir(const graph::module_t& module) const;
    filesystem::path_t artifact_dir(const graph::module_t& module) const;
    filesystem::path_t artifact_alias_dir(const graph::module_t& module) const;

    filesystem::path_t builder_source_path(const graph::module_t& module) const;
    filesystem::path_t builder_dir(const graph::module_t& module) const;
    filesystem::path_t builder_build_dir(const graph::module_t& module) const;
    filesystem::path_t builder_install_dir(const graph::module_t& module) const;
    filesystem::path_t builder_install_path(const graph::module_t& module) const;

    filesystem::path_t interface_dir(const graph::module_t& module) const;
    filesystem::path_t interface_build_dir(const graph::module_t& module, library_type_t library_type) const;
    filesystem::path_t interface_install_dir(const graph::module_t& module, library_type_t library_type) const;

    filesystem::path_t libraries_dir(const graph::module_t& module) const;
    filesystem::path_t libraries_build_dir(const graph::module_t& module, library_type_t library_type) const;
    filesystem::path_t libraries_install_dir(const graph::module_t& module, library_type_t library_type) const;

    filesystem::path_t import_dir(const graph::module_t& module) const;
    filesystem::path_t import_build_dir(const graph::module_t& module) const;
    filesystem::path_t import_install_dir(const graph::module_t& module) const;

    filesystem::path_t phase_dir(const graph::module_t& module, phase_t phase) const;
    filesystem::path_t phase_build_dir(const graph::module_t& module, phase_t phase, library_type_t library_type) const;
    filesystem::path_t phase_install_dir(const graph::module_t& module, phase_t phase, library_type_t library_type) const;

    filesystem::path_t build_builder(graph::module_t& module) const;
    filesystem::relative_path_t build_relative_dir() const;
    filesystem::relative_path_t install_relative_dir() const;
    filesystem::relative_path_t library_type_relative_dir(library_type_t library_type) const;

private:
    graph::workspace_ecosystem_t& m_workspace_ecosystem;
    graph::module_t& m_module;
};

BUILDER_EXTERN void module_builder__export_interface(const module_builder_t* module_builder, library_type_t library_type);
BUILDER_EXTERN void module_builder__export_libraries(const module_builder_t* module_builder, library_type_t library_type);
BUILDER_EXTERN void module_builder__import_libraries(const module_builder_t* module_builder);

} // namespace builder

} // namespace cpp_builder

} // namespace kernel

#endif // KERNEL_CPP_BUILDER_MODULE_BUILDER_H
