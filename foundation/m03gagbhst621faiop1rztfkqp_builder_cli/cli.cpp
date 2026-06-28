#include <m03gagbhst621faiop1rztfkqp_builder_cli/builder_cli.h>

#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <iostream>
#include <exception>
#include <format>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << std::format("usage: {} <module> [args...]", argv[0]) << std::endl;
        return 1;
    }

    try {
        const auto module = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t(argv[1]);

        std::vector<m03gagbhsvr0m5w15urj0o291m_process::process_arg_t> args;
        for (int i = 2; i < argc; ++i) {
            args.push_back(argv[i]);
        }

        m03gagbhst621faiop1rztfkqp_builder_cli::run(module, args);
    } catch (const std::exception& e) {
        std::cout << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
