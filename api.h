#ifndef KERNEL_API_H
# define KERNEL_API_H

# include "compiler.h"
# include "graph.h"
# include "phase.h"

namespace kernel {

template <class phase_t>
typename phase_t::output_t build(graph::module_t& module, module_config::module_config_t module_config) {
    phase::output_artifacts_t output {
        .root = filesystem::path_t("/"),
        .artifacts = {}
    };
    const phase_t requested_phase(module, module_config, output);
    return requested_phase.template build<phase_t>();
}

std::vector<filesystem::path_t> find(
    const phase::output_artifacts_t& output,
    const filesystem::find_include_predicate_t& include_predicate
);

std::vector<filesystem::path_t> find(
    const phase::binary_phase_t::output_t& output,
    const filesystem::find_include_predicate_t& include_predicate
);

phase::output_artifact_t find(
    const phase::output_artifacts_t& output,
    const filesystem::relative_path_t& relative_path
);

phase::output_artifact_t find(
    const phase::binary_phase_t::output_t& output,
    const filesystem::relative_path_t& relative_path
);

filesystem::path_t create_library(
    const phase::library_phase_t& phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<binding::binding_t>& defines,
    const filesystem::relative_path_t& relative_output_path
);

phase::default_library_t default_library(
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<binding::binding_t>& defines
);

filesystem::path_t create_binary(
    const phase::binary_phase_t& phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<binding::binding_t>& defines,
    const filesystem::relative_path_t& relative_output_path
);

phase::default_cli_t default_cli(
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<binding::binding_t>& defines
);

} // namespace kernel

#endif // KERNEL_API_H
