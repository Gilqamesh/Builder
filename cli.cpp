#include "graph.h"
#include "module_builder.h"
#include "process.h"

#include <iostream>
#include <format>
#include <vector>
#include <set>
#include <fstream>

using namespace kernel::cpp_builder;

void render_graph_svg(const graph::workspace_ecosystem_t& ecosystem, const std::filesystem::path& out_svg_path) {
    std::set<int> levels;
    std::vector<graph::module_t*> modules;
    for (const auto& [_, workspace] : ecosystem.workspace_by_workspace_relative_path_to_workspace_ecosystem) {
        levels.insert(workspace->level);
        for (auto& [_, module] : workspace->module_by_module_relative_path_to_workspace) {
            modules.push_back(module);
        }
    }

    const auto out_dir = out_svg_path.parent_path();
    if (!out_dir.empty()) std::filesystem::create_directories(out_dir);

    const auto dot_file = out_svg_path.string() + ".dot";

    const auto module_relative_path_str = [](const graph::module_t* module) {
        return module->workspace->workspace_relative_path_to_workspace_ecosystem.string() + "_" + module->module_relative_path_to_workspace.string();
    };

    auto pid = [&](const graph::module_t& m) { return std::format("P{}", module_relative_path_str(&m)); };
    auto mid = [&](const graph::module_t& m) { return std::format("M{}", module_relative_path_str(&m)); };

    {
        std::ofstream f(dot_file, std::ios::binary);
        if (!f) throw std::runtime_error(std::format("Failed to open DOT file: {}", dot_file));

        f << "digraph G {\n";
        f << "  graph [rankdir=LR, splines=true, nodesep=0.6, ranksep=1.6, bgcolor=\"white\"];\n";
        f << "  node  [fontname=\"Inter,Helvetica,Arial\", fontsize=11];\n";
        f << "  edge  [color=\"#333333\", arrowsize=0.8];\n\n";

        // one cluster per stratum (clear boundary)
        for (int L : levels) {
            f << std::format("  subgraph cluster_L{} {{\n", L);
            f << std::format("    label=\"L{}\";\n", L);
            f << "    labelloc=\"t\";\n";
            f << "    fontsize=12;\n";
            f << "    color=\"#BBBBBB\";\n";
            f << "    style=\"rounded\";\n\n";

            for (auto* m : modules) {
                if (m->workspace->level != L) continue;

                f << std::format("    {} [shape=ellipse, style=filled, fillcolor=\"#FFF3E6\", label=\"P{}\"];\n",
                                 pid(*m), module_relative_path_str(m));
                f << std::format("    {} [shape=box, style=\"rounded,filled\", fillcolor=\"#F3F6FF\", label=\"M{}\"];\n",
                                 mid(*m), module_relative_path_str(m));
            }

            f << "  }\n\n";
        }

        // edges: show everything with same arrow style
        // module deps: dep -> module
        for (auto* dst : modules)
            for (auto* dep : dst->dependencies)
                f << std::format("  {} -> {};\n", mid(*dep), mid(*dst));

        // producer deps: dep -> producer
        for (auto* dst : modules)
            for (auto* dep : dst->module_builder->dependencies)
                f << std::format("  {} -> {};\n", mid(*dep), pid(*dst));

        // produce relation (producer -> module)
        for (auto* m : modules)
            f << std::format("  {} -> {};\n", pid(*m), mid(*m));

        f << "}\n";
    }

    {
        const std::string cmd = std::format(
            "dot -Tsvg \"{}\" -o \"{}\"",
            dot_file,
            out_svg_path.string()
        );

        if (std::system(cmd.c_str()) != 0) {
            throw std::runtime_error(std::format("Graphviz render failed: {}", cmd));
        }
    }

    {
        const auto png_path = out_svg_path.string() + ".png";
        const std::string cmd = std::format(
            "rsvg-convert \"{}\" -o \"{}\"",
            out_svg_path.string(),
            png_path
        );

        if (std::system(cmd.c_str()) != 0) {
            throw std::runtime_error(std::format("rsvg-convert failed: {}", cmd));
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << std::format("usage: {} <workspace_ecosystem_path> <workspace_relative_path> <module_relative_path> [binary_relative_path] [args...]", argv[0]) << std::endl;
        return 1;
    }

    try {
        const auto workspace_ecosystem_path = filesystem::path_t(argv[1]);
        const auto workspace_relative_path = filesystem::relative_path_t(argv[2]);
        const auto module_relative_path = filesystem::relative_path_t(argv[3]);

        graph::workspace_ecosystem_t* workspace_ecosystem = new graph::workspace_ecosystem_t {
            .absolute_path_to_workspace_directory = workspace_ecosystem_path,
            .artifact_dir = workspace_ecosystem_path / filesystem::relative_path_t("artifacts")
        };

        graph::module_t* target_module = workspace_ecosystem->discover_module(workspace_relative_path, module_relative_path);

        target_module->validate();

        builder::module_builder_t kernel_module_builder(*workspace_ecosystem, *workspace_ecosystem->this_module);
        builder::module_builder_t target_module_builder(*workspace_ecosystem, *target_module);

        const auto cli = filesystem::canonical(filesystem::path_t("/proc/self/exe"));
        const auto cli_last_write_time = filesystem::last_write_time(cli);
        const auto cli_version = graph::derive_version(cli_last_write_time);

        if (!filesystem::exists(kernel_module_builder.import_install_dir()) || cli_version.value < workspace_ecosystem->this_module->version.value) {
            kernel_module_builder.import_libraries();

            const auto new_cli = kernel_module_builder.import_install_dir() / filesystem::relative_path_t("cli");
            if (!filesystem::exists(new_cli)) {
                throw std::runtime_error(std::format("kernel::cpp_builder::cli: expected updated '{}' to exist but it does not", new_cli));
            }

            std::vector<process::process_arg_t> process_args;
            process_args.push_back(new_cli);
            for (int i = 1; i < argc; ++i) {
                process_args.push_back(argv[i]);
            }
            process::exec(process_args);
        }

        render_graph_svg(*workspace_ecosystem, workspace_relative_path.string() + module_relative_path.string() + ".svg");

        target_module_builder.import_libraries();

        if (4 < argc) {
            const auto binary = filesystem::relative_path_t(argv[4]);
            const auto binary_dir = target_module_builder.import_install_dir();
            const auto binary_location = binary_dir / binary;
            if (!filesystem::exists(binary_location)) {
                throw std::runtime_error(std::format("kernel::cpp_builder::cli: binary '{}' at location '{}' does not exist", binary, binary_location));
            }

            filesystem::current_path(binary_dir);

            std::vector<process::process_arg_t> process_args;
            process_args.push_back(binary.string());
            for (int i = 5; i < argc; ++i) {
                process_args.push_back(argv[i]);
            }
            process::exec(process_args);
        }
    } catch (const std::exception& e) {
        std::cout << std::format("{}: {}", argv[0], e.what()) << std::endl;
        return 1;
    }

    return 0;
}
