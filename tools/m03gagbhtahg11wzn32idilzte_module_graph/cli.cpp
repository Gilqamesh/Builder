#include "module_graph.h"

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

#include <exception>
#include <format>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " <target-module> <output-svg>\n";
        return 1;
    }

    try {
        const auto invocation_context = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context();
        m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t workspace_graph(
            invocation_context.workspace_root,
            invocation_context.artifact_root
        );
        auto* target_module = workspace_graph.discover_module(m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t(argv[1]));

        const auto output_svg = module_graph::render_svg(
            workspace_graph,
            *target_module,
            m03gagbhsnusi43zogoacgj2ez_filesystem::path_t(argv[2])
        );
        std::cout << output_svg.string() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
