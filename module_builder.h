#ifndef KERNEL_CPP_BUILDER_MODULE_BUILDER_H
# define KERNEL_CPP_BUILDER_MODULE_BUILDER_H

# include "graph.h"

# include <cstdint>
# include <format>
# include <stdexcept>
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
};

struct source_output_t {
    std::vector<filesystem::path_t> source_roots;
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

    template <class phase_t>
    const typename phase_t::output_t& materialize() const;

protected:
    module_builder_t& module_builder() const;

private:
    std::string_view m_name;
    module_builder_t& m_module_builder;
    graph::module_t& m_module;
    const iphase_t* m_predecessor;
};

struct source_phase_t : phase_base_t {
    using output_t = source_output_t;

    source_phase_t(module_builder_t& module_builder, graph::module_t& module, const iphase_t* predecessor = nullptr);

    filesystem::path_t dir() const override;
    filesystem::path_t build_dir() const override;
    filesystem::path_t install_dir() const override;

private:
    friend class phase_base_t;

    void execute() const;
    const output_t& output() const;

    mutable output_t m_output;
};

struct export_interface_phase_t : phase_base_t {
    using output_t = export_interface_output_t;

    export_interface_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor = nullptr);

    filesystem::path_t dir() const override;
    filesystem::path_t build_dir() const override;
    filesystem::path_t install_dir() const override;

public:
    const library_type_t library_type;

private:
    friend class phase_base_t;

    void execute() const;
    const output_t& output() const;

    mutable output_t m_output;
};

struct export_libraries_phase_t : phase_base_t {
    using output_t = export_libraries_output_t;

    export_libraries_phase_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type, const iphase_t* predecessor = nullptr);
    filesystem::path_t dir() const override;
    filesystem::path_t build_dir() const override;
    filesystem::path_t install_dir() const override;

public:
    const library_type_t library_type;

private:
    friend class phase_base_t;

    void execute() const;
    const output_t& output() const;

    mutable output_t m_output;
};

struct import_libraries_phase_t : phase_base_t {
    using output_t = import_libraries_output_t;

    import_libraries_phase_t(module_builder_t& module_builder, graph::module_t& module, const iphase_t* predecessor = nullptr);
    filesystem::path_t dir() const override;
    filesystem::path_t build_dir() const override;
    filesystem::path_t install_dir() const override;

private:
    friend class phase_base_t;

    void execute() const;
    const output_t& output() const;

    mutable output_t m_output;
};

class phase_chain_t {
public:
    phase_chain_t(module_builder_t& module_builder, graph::module_t& module, library_type_t library_type);

    source_phase_t source;
    export_interface_phase_t export_interface;
    export_libraries_phase_t export_libraries;
    import_libraries_phase_t import_libraries;
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
    filesystem::path_t artifact_latest_dir() const;

    filesystem::path_t source_phase_dir() const;
    filesystem::path_t source_phase_build_dir() const;
    filesystem::path_t source_phase_install_dir() const;

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
    friend class phase_base_t;
    friend struct source_phase_t;
    friend struct export_interface_phase_t;
    friend struct export_libraries_phase_t;
    friend struct import_libraries_phase_t;

    void run_phase(graph::module_t& module, phase_t phase, library_type_t library_type) const;

    void dispatch_phase(const export_interface_phase_t& phase) const;
    void dispatch_phase(const export_libraries_phase_t& phase) const;
    void dispatch_phase(const import_libraries_phase_t& phase) const;

    void run_module_producer_phase(const export_interface_phase_t& phase) const;
    void run_module_producer_phase(const export_libraries_phase_t& phase) const;
    void run_module_producer_phase(const import_libraries_phase_t& phase) const;

    void run_kernel_phase(const export_interface_phase_t& phase) const;
    void run_kernel_phase(const export_libraries_phase_t& phase) const;
    void run_kernel_phase(const import_libraries_phase_t& phase) const;

    filesystem::path_t source_dir(const graph::module_t& module) const;
    filesystem::path_t artifact_base_dir(const graph::module_t& module) const;
    filesystem::path_t artifact_dir(const graph::module_t& module) const;
    filesystem::path_t artifact_latest_dir(const graph::module_t& module) const;
    void publish_latest_stage(const iphase_t& phase) const;

    filesystem::path_t source_phase_dir(const graph::module_t& module) const;
    filesystem::path_t source_phase_build_dir(const graph::module_t& module) const;
    filesystem::path_t source_phase_install_dir(const graph::module_t& module) const;

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

    filesystem::path_t build_builder(graph::module_t& module) const;
    filesystem::relative_path_t build_relative_dir() const;
    filesystem::relative_path_t install_relative_dir() const;
    filesystem::relative_path_t library_type_relative_dir(library_type_t library_type) const;

private:
    graph::workspace_ecosystem_t& m_workspace_ecosystem;
    graph::module_t& m_module;
};

template <class phase_t>
const typename phase_t::output_t& phase_base_t::materialize() const {
    const auto* phase = dynamic_cast<const phase_t*>(this);
    if (phase == nullptr) {
        const auto* previous_phase = predecessor();
        if (previous_phase == nullptr) {
            throw std::runtime_error(std::format("kernel::cpp_builder::builder::phase_base_t::materialize: requested phase is not in the current phase '{}' or its configured predecessor chain", name()));
        }

        const auto* previous_phase_base = dynamic_cast<const phase_base_t*>(previous_phase);
        if (previous_phase_base == nullptr) {
            throw std::runtime_error(std::format("kernel::cpp_builder::builder::phase_base_t::materialize: predecessor of phase '{}' is not a phase_base_t", name()));
        }

        return previous_phase_base->materialize<phase_t>();
    }

    const auto root_dir = dir();
    const auto build_dir = this->build_dir();
    const auto install_dir = this->install_dir();
    const auto marker_path = [&](std::string_view state) {
        return build_dir / filesystem::relative_path_t(std::format("{}.{}", name(), state));
    };
    const auto started_marker = marker_path("started");
    const auto complete_marker = marker_path("complete");

    if (filesystem::exists(complete_marker)) {
        m_module_builder.publish_latest_stage(*this);
        return phase->output();
    }

    if (filesystem::exists(started_marker)) {
        throw std::runtime_error(std::format("kernel::cpp_builder::builder::phase_base_t::materialize: re-entry detected for phase '{}'", name()));
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

        phase->execute();
        const auto& result = phase->output();

        m_module_builder.publish_latest_stage(*this);
        filesystem::touch(complete_marker);
        filesystem::remove(started_marker);

        return result;
    } catch (...) {
        filesystem::remove_all(root_dir);
        throw ;
    }
}

BUILDER_EXTERN void module_builder__export_interface(const export_interface_phase_t* phase);
BUILDER_EXTERN void module_builder__export_libraries(const export_libraries_phase_t* phase);
BUILDER_EXTERN void module_builder__import_libraries(const import_libraries_phase_t* phase);

} // namespace builder

} // namespace cpp_builder

} // namespace kernel

#endif // KERNEL_CPP_BUILDER_MODULE_BUILDER_H
