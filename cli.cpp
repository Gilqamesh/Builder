#include "api.h"
#include "graph.h"
#include "process.h"

#include <iostream>
#include <format>

using namespace kernel;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << std::format("usage: {} <workspace-root> <module> [args...]", argv[0]) << std::endl;
        return 1;
    }

    try {
        const auto workspace_root = filesystem::path_t(argv[1]);
        const auto module = filesystem::relative_path_t(argv[2]);

        graph::workspace_graph_t* workspace_graph = new graph::workspace_graph_t(
            workspace_root,
            workspace_root / filesystem::relative_path_t("artifacts")
        );

        graph::module_t* target_module = workspace_graph->discover_module(module);

        const auto cli = filesystem::canonical(filesystem::path_t("/proc/self/exe"));
        const auto cli_last_write_time = filesystem::last_write_time(cli);
        const auto cli_version = graph::derive_version(cli_last_write_time);

        if (cli_version.value < workspace_graph->kernel_module().version().value) {
            const auto kernel_binary_output = build<phase::binary_phase_t>(
                workspace_graph->kernel_module(),
                module_config::module_config_t { .library_type = module_config::library_type_t::SHARED }
            );

            std::vector<process::process_arg_t> process_args;
            process_args.push_back(kernel_binary_output.cli.path);
            for (int i = 1; i < argc; ++i) {
                process_args.push_back(argv[i]);
            }
            process::exec(process_args);
        }

        const auto target_binary_output = build<phase::binary_phase_t>(
            *target_module,
            module_config::module_config_t { .library_type = module_config::library_type_t::SHARED }
        );

        filesystem::current_path(target_binary_output.root);

        std::vector<process::process_arg_t> process_args;
        process_args.push_back(target_binary_output.cli.relative_path.string());
        for (int i = 3; i < argc; ++i) {
            process_args.push_back(argv[i]);
        }
        process::exec(process_args);
    } catch (const std::exception& e) {
        std::cout << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
