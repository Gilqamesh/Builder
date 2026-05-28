#ifndef KERNEL_CPP_BUILDER_MODULE_BUILDER_H
# define KERNEL_CPP_BUILDER_MODULE_BUILDER_H

# include "graph.h"
# include "phase.h"

# include <vector>

namespace kernel {

namespace cpp_builder {

namespace builder {

class module_builder_t {
public:
    module_builder_t(graph::workspace_ecosystem_t& workspace_ecosystem, graph::module_t& module);

    graph::module_t& module() const;

private:
    friend class phase_base_t;
    friend struct source_phase_t;
    friend struct interface_phase_t;
    friend struct library_phase_t;
    friend struct binary_phase_t;

    std::vector<filesystem::path_t> interface_roots(library_type_t library_type) const;
    std::vector<std::vector<filesystem::path_t>> library_groups(library_type_t library_type) const;

    filesystem::path_t source_dir(const graph::module_t& module) const;

    filesystem::path_t builder_source_path() const;
    filesystem::path_t builder_dir() const;
    filesystem::path_t builder_build_dir() const;
    filesystem::path_t builder_install_dir() const;
    filesystem::path_t builder_install_path() const;

    filesystem::path_t artifact_base_dir(const graph::module_t& module) const;
    filesystem::path_t artifact_dir(const graph::module_t& module) const;
    filesystem::path_t artifact_latest_dir(const graph::module_t& module) const;
    void publish_latest_stage(const iphase_t& phase) const;

    filesystem::path_t builder_source_path(const graph::module_t& module) const;
    filesystem::path_t builder_dir(const graph::module_t& module) const;
    filesystem::path_t builder_build_dir(const graph::module_t& module) const;
    filesystem::path_t builder_install_dir(const graph::module_t& module) const;
    filesystem::path_t builder_install_path(const graph::module_t& module) const;

    filesystem::path_t build_builder(graph::module_t& module) const;
    filesystem::relative_path_t build_relative_dir() const;
    filesystem::relative_path_t install_relative_dir() const;
    filesystem::relative_path_t library_type_relative_dir(library_type_t library_type) const;

private:
    graph::workspace_ecosystem_t& m_workspace_ecosystem;
    graph::module_t& m_module;
};

} // namespace builder

} // namespace cpp_builder

} // namespace kernel

#endif // KERNEL_CPP_BUILDER_MODULE_BUILDER_H
