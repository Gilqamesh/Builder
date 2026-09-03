#include "builder_cli.h"

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>
#include <m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.h>
#include <m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed/bootstrap_seed.h>

#include <string_view>

namespace m03gagbhst621faiop1rztfkqp_builder_cli {

namespace bootstrap_seed = m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed;

static constexpr std::string_view DEFAULT_TARGET = "cli";

static m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t::installed_t install_binary_phase(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    std::string_view target
) {
    const auto phase = m03gagbhsujjf63n0w3r2w4q6h_build_phases::phase_base_t::make(module, target);
    return phase->install<m03gagbhsujjf63n0w3r2w4q6h_build_phases::binary_phase_t>();
}

static bool current_cli_is_older_than_bootstrap_seed(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph) {
    const auto cli = m03gagbhsnusi43zogoacgj2ez_filesystem::canonical(m03gagbhsnusi43zogoacgj2ez_filesystem::path_t("/proc/self/exe"));
    const auto cli_last_write_time = m03gagbhsnusi43zogoacgj2ez_filesystem::last_write_time(cli);
    const auto cli_version = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::version_t(cli_last_write_time);

    return cli_version.value < bootstrap_seed::version(workspace_graph).value;
}

static std::string module_target_argument(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t& module,
    std::string_view target
) {
    if (target == DEFAULT_TARGET) {
        return module.unique_name();
    }

    std::string result = module.unique_name();
    result.push_back(':');
    result.append(target);
    return result;
}

static m03gagbhsvr0m5w15urj0o291m_process::command_t build_cli_command(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t& module,
    std::string_view target,
    const std::vector<std::string>& additional_module_args
) {
    const auto invocation_context = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context();
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t workspace_graph(
        invocation_context.workspace_root,
        invocation_context.artifact_root
    );

    workspace_graph.discover_module(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t("m03gagbhst621faiop1rztfkqp_builder_cli"));

    if (current_cli_is_older_than_bootstrap_seed(workspace_graph)) {
        const auto bootstrap_seed_binary = install_binary_phase(bootstrap_seed::module(workspace_graph), DEFAULT_TARGET);

        std::vector<std::string> process_args;
        process_args.push_back(bootstrap_seed_binary.target(DEFAULT_TARGET).string());
        process_args.push_back(module_target_argument(module, target));
        process_args.insert(process_args.end(), additional_module_args.begin(), additional_module_args.end());

        return m03gagbhsvr0m5w15urj0o291m_process::command_t(process_args);
    }

    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t* target_module = workspace_graph.discover_module(module);
    const auto target_binary = install_binary_phase(*target_module, target);
    const auto target_path = target_binary.target(target);

    std::vector<std::string> process_args;
    process_args.push_back(target_path.string());
    process_args.insert(process_args.end(), additional_module_args.begin(), additional_module_args.end());

    return m03gagbhsvr0m5w15urj0o291m_process::command_t(process_args, target_path.parent());
}

[[noreturn]] void exec(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, int argc, const char* const* argv) {
    exec(module, DEFAULT_TARGET, argc, argv);
}

[[noreturn]] void exec(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::string_view target, int argc, const char* const* argv) {
    m03gagbhsvr0m5w15urj0o291m_process::exec(build_cli_command(module, target, std::vector<std::string>(argv, argv + argc)));
}

[[noreturn]] void exec(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::vector<std::string> args) {
    exec(module, DEFAULT_TARGET, args);
}

[[noreturn]] void exec(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::string_view target, std::vector<std::string> args) {
    m03gagbhsvr0m5w15urj0o291m_process::exec(build_cli_command(module, target, args));
}

void create_and_wait_checked(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, int argc, const char* const* argv) {
    create_and_wait_checked(module, DEFAULT_TARGET, argc, argv);
}

void create_and_wait_checked(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::string_view target, int argc, const char* const* argv) {
    m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_foreground_checked(build_cli_command(module, target, std::vector<std::string>(argv, argv + argc)));
}

void create_and_wait_checked(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::vector<std::string> args) {
    create_and_wait_checked(module, DEFAULT_TARGET, args);
}

void create_and_wait_checked(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module, std::string_view target, std::vector<std::string> args) {
    m03gagbhsvr0m5w15urj0o291m_process::create_and_wait_foreground_checked(build_cli_command(module, target, args));
}

} // namespace m03gagbhst621faiop1rztfkqp_builder_cli
