#include "builder_cli.h"

#include <m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain/cxx_toolchain.h>
#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>
#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>

namespace m03gagbhst621faiop1rztfkqp_builder_cli {

static m03gagbhsujjf63n0w3r2w4q6h_build_phases::build_config_t build_build_config() {
    return m03gagbhsujjf63n0w3r2w4q6h_build_phases::build_config_t {
        .library_type = m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain::library_type_t::SHARED
    };
}

static m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t::installed_t install_cli(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module) {
    const auto phase = m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::make(module, build_build_config());
    return phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t>();
}

static bool current_cli_is_older_than_bootstrap_seed(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph) {
    const auto cli = m03gagbhsnusi43zogoacgj2ez_filesystem::canonical(m03gagbhsnusi43zogoacgj2ez_filesystem::path_t("/proc/self/exe"));
    const auto cli_last_write_time = m03gagbhsnusi43zogoacgj2ez_filesystem::last_write_time(cli);
    const auto cli_version = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::version_t(cli_last_write_time);

    return cli_version.value < workspace_graph.bootstrap_seed_module().version().value;
}

static m03gagbhsvr0m5w15urj0o291m_process::command_t build_cli_command(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t& module,
    const std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t>& args
) {
    const auto invocation_context = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context();
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t workspace_graph(
        invocation_context.workspace_root,
        invocation_context.artifact_root
    );

    workspace_graph.discover_module(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t("m03gagbhst621faiop1rztfkqp_builder_cli"));

    if (current_cli_is_older_than_bootstrap_seed(workspace_graph)) {
        const auto bootstrap_seed_binary = install_cli(workspace_graph.bootstrap_seed_module());

        std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t> process_args;
        process_args.push_back(bootstrap_seed_binary.cli());
        process_args.push_back(module.unique_name());
        process_args.insert(process_args.end(), args.begin(), args.end());

        return m03gagbhsvr0m5w15urj0o291m_process::command_t {
            .args = process_args
        };
    }

    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t* target_module = workspace_graph.discover_module(module);
    const auto target_binary = install_cli(*target_module);

    std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t> process_args;
    process_args.push_back(target_binary.cli());
    process_args.insert(process_args.end(), args.begin(), args.end());

    return m03gagbhsvr0m5w15urj0o291m_process::command_t {
        .args = process_args,
        .working_dir = target_binary.root()
    };
}

[[noreturn]] void exec(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, const std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t>& args) {
    m03gagbhsvr0m5w15urj0o291m_process::exec(build_cli_command(module, args));
}

void create_and_wait_checked(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, const std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t>& args) {
    m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_checked(build_cli_command(module, args));
}

} // namespace m03gagbhst621faiop1rztfkqp_builder_cli
