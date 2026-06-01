#ifndef KERNEL_API_H
# define KERNEL_API_H

# include "compiler.h"
# include "graph.h"
# include "phase.h"

namespace kernel {

namespace phase {

template <class phase_t>
output_t build(graph::module_t& module, module_config::module_config_t module_config) {
    output_t output {
        .root = filesystem::path_t("/"),
        .artifacts = {}
    };
    const phase_t requested_phase(module, module_config, output);
    return requested_phase.template build<phase_t>();
}

} // namespace phase

namespace filesystem {

std::vector<path_t> find(
    const phase::output_t& output,
    const find_include_predicate_t& include_predicate
);

phase::output_artifact_t find(
    const phase::output_t& output,
    const relative_path_t& relative_path
);

} // namespace filesystem

namespace compiler {

filesystem::path_t create_library(
    const phase::library_phase_t& phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<define_t>& defines,
    const filesystem::relative_path_t& relative_output_path
);

filesystem::path_t create_binary(
    const phase::binary_phase_t& phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<define_t>& defines,
    const filesystem::relative_path_t& relative_output_path
);

} // namespace compiler

} // namespace kernel

#endif // KERNEL_API_H
