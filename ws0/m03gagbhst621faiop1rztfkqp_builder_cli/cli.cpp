#include <m03gagbhst621faiop1rztfkqp_builder_cli/builder_cli.h>

#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
#include <m03gagbhsvr0m5w15urj0o291m_process/process.h>

#include <iostream>
#include <exception>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

struct module_target_t {
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t module;
    std::string target;
};

static module_target_t parse_module_target(std::string_view value) {
    constexpr std::string_view DEFAULT_TARGET = "cli";
    const auto delimiter = value.find(':');

    if (delimiter == std::string_view::npos) {
        return {
            m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t(value),
            std::string(DEFAULT_TARGET)
        };
    }

    if (delimiter == 0 || delimiter + 1 == value.size()) {
        throw std::invalid_argument(std::format("module target '{}' must be <module>[:target]", value));
    }

    return {
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t(value.substr(0, delimiter)),
        std::string(value.substr(delimiter + 1))
    };
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << std::format("usage: {} <module>[:target] [args...]", argv[0]) << std::endl;
        return 1;
    }

    try {
        const auto module_target = parse_module_target(argv[1]);
        m03gagbhst621faiop1rztfkqp_builder_cli::exec(module_target.module, module_target.target, argc - 2, argv + 2);
    } catch (const std::exception& e) {
        std::cout << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
