#include "api.h"
#include "graph.h"
#include "process.h"

#include <iostream>
#include <format>

using namespace kernel;

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << std::format("usage: {} <workspace-root> <workspace> <module> [args...]", argv[0]) << std::endl;
        return 1;
    }

    try {
        const auto workspace_root = filesystem::path_t(argv[1]);
        const auto workspace = filesystem::relative_path_t(argv[2]);
        const auto module = filesystem::relative_path_t(argv[3]);

        graph::workspace_ecosystem_t* workspace_ecosystem = new graph::workspace_ecosystem_t {
            .workspace_root = workspace_root,
            .artifact_dir = workspace_root / filesystem::relative_path_t("artifacts")
        };

        graph::module_t* target_module = workspace_ecosystem->discover_module(workspace, module);

        const auto cli = filesystem::canonical(filesystem::path_t("/proc/self/exe"));
        const auto cli_last_write_time = filesystem::last_write_time(cli);
        const auto cli_version = graph::derive_version(cli_last_write_time);

        if (cli_version.value < workspace_ecosystem->this_module->version.value) {
            const auto kernel_binary_output = phase::build<phase::binary_phase_t>(
                *workspace_ecosystem->this_module,
                module_config::module_config_t { .library_type = module_config::library_type_t::SHARED }
            );
            const auto new_cli_path = kernel_binary_output.root / filesystem::relative_path_t("cli");
            const auto new_cli_matches = kernel::filesystem::find(kernel_binary_output, filesystem::find_include_predicate_t::path(new_cli_path));
            if (new_cli_matches.empty()) {
                throw std::runtime_error(std::format("kernel::cli: expected updated '{}' in binary output but it does not exist", new_cli_path));
            }

            std::vector<process::process_arg_t> process_args;
            process_args.push_back(new_cli_matches.front());
            for (int i = 1; i < argc; ++i) {
                process_args.push_back(argv[i]);
            }
            process::exec(process_args);
        }

        const auto target_binary_output = phase::build<phase::binary_phase_t>(
            *target_module,
            module_config::module_config_t { .library_type = module_config::library_type_t::SHARED }
        );
        const auto default_cli = filesystem::relative_path_t("cli");
        const auto default_cli_location = target_binary_output.root / default_cli;
        const auto default_cli_matches = kernel::filesystem::find(target_binary_output, filesystem::find_include_predicate_t::path(default_cli_location));
        if (default_cli_matches.empty()) {
            throw std::runtime_error(std::format("kernel::cli: default CLI '{}' at location '{}' does not exist in binary output", default_cli, default_cli_location));
        }

        filesystem::current_path(target_binary_output.root);

        std::vector<process::process_arg_t> process_args;
        process_args.push_back(default_cli.string());
        for (int i = 4; i < argc; ++i) {
            process_args.push_back(argv[i]);
        }
        process::exec(process_args);
    } catch (const std::exception& e) {
        std::cout << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
