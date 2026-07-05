#include <m03gagbhtahg11wzn32idilzte_module_graph/module_graph.h>

#include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
#include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
#include <m03gagbht6ja46uikb1ltan0x8_dot/dot.h>

#include <fstream>
#include <format>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace m03gagbhtahg11wzn32idilzte_module_graph {

static std::string dot_string(std::string_view value) {
    std::string result = "\"";

    for (const char c : value) {
        if (c == '\n') {
            result += "\\n";
            continue ;
        }
        if (c == '\\' || c == '"') {
            result.push_back('\\');
        }
        result.push_back(c);
    }

    result.push_back('"');
    return result;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t write_dot(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& target_module,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_dot_path
) {
    if (output_dot_path.extension() != ".dot") {
        throw std::runtime_error(std::format("module_graph::write_dot: output path '{}' must have .dot extension", output_dot_path));
    }

    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(output_dot_path)) {
        throw std::runtime_error(std::format("module_graph::write_dot: output path '{}' already exists", output_dot_path));
    }

    const auto output_parent = output_dot_path.parent();
    if (!m03gagbhsnusi43zogoacgj2ez_filesystem::exists(output_parent)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::create_directories(output_parent);
    }

    const auto modules = workspace_graph.modules();
    std::unordered_map<const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t*, std::string> builder_node_by_module;
    std::unordered_map<const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t*, std::string> module_node_by_module;

    for (std::size_t i = 0; i < modules.size(); ++i) {
        builder_node_by_module.emplace(modules[i], std::format("p{}", i));
        module_node_by_module.emplace(modules[i], std::format("m{}", i));
    }

    std::ofstream ofs(output_dot_path.string(), std::ios::binary | std::ios::trunc);
    if (!ofs) {
        throw std::runtime_error(std::format("module_graph::write_dot: failed to open output file '{}'", output_dot_path));
    }

    ofs << "digraph BuilderModuleGraph {\n";
    ofs << "  graph [rankdir=LR, splines=true, nodesep=0.6, ranksep=1.4, bgcolor=\"white\"];\n";
    ofs << "  node [fontname=\"Inter,Helvetica,Arial\", fontsize=11];\n";
    ofs << "  edge [color=\"#333333\", arrowsize=0.8];\n\n";

    for (const auto* workspace : workspace_graph.workspaces()) {
        ofs << std::format("  subgraph cluster_workspace_{} {{\n", workspace->name().order_position());
        ofs << std::format("    label={};\n", dot_string(workspace->name().relative_path().string()));
        ofs << "    labelloc=\"t\";\n";
        ofs << "    fontsize=12;\n";
        ofs << "    color=\"#BBBBBB\";\n";
        ofs << "    style=\"rounded\";\n\n";

        for (const auto* module : modules) {
            if (&module->workspace() != workspace) {
                continue ;
            }

            const auto builder_node = builder_node_by_module.at(module);
            const auto module_node = module_node_by_module.at(module);
            const auto label = module->name().unique_name();
            const auto target_style = module == &target_module ? ", penwidth=2.2, color=\"#111111\"" : "";

            ofs << std::format(
                "    {} [shape=ellipse, style=filled, fillcolor=\"#FFF3E6\", label={}{}];\n",
                builder_node,
                dot_string(std::format("builder\n{}", label)),
                target_style
            );
            ofs << std::format(
                "    {} [shape=box, style=\"rounded,filled\", fillcolor=\"#F3F6FF\", label={}{}];\n",
                module_node,
                dot_string(std::format("module\n{}", label)),
                target_style
            );
        }

        ofs << "  }\n\n";
    }

    for (auto* module : modules) {
        const auto module_node = module_node_by_module.at(module);
        const auto builder_node = builder_node_by_module.at(module);

        for (const auto* dependency : module->dependencies()) {
            ofs << std::format("  {} -> {} [label=\"module\"];\n", module_node_by_module.at(dependency), module_node);
        }

        for (const auto* dependency : module->builder_dependencies()) {
            ofs << std::format("  {} -> {} [label=\"builder\"];\n", module_node_by_module.at(dependency), builder_node);
        }

        ofs << std::format("  {} -> {} [label=\"builds\"];\n", builder_node, module_node);
    }

    ofs << "}\n";

    if (!ofs) {
        throw std::runtime_error(std::format("module_graph::write_dot: failed while writing output file '{}'", output_dot_path));
    }

    return output_dot_path;
}

m03gagbhsnusi43zogoacgj2ez_filesystem::path_t render_svg(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& target_module,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_svg_path
) {
    if (output_svg_path.extension() != ".svg") {
        throw std::runtime_error(std::format("module_graph::render_svg: output path '{}' must have .svg extension", output_svg_path));
    }

    const auto dot_path = output_svg_path + "_tmp.dot";
    if (m03gagbhsnusi43zogoacgj2ez_filesystem::exists(dot_path)) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(dot_path);
    }

    try {
        write_dot(workspace_graph, target_module, dot_path);
        const auto result = m03gagbht6ja46uikb1ltan0x8_dot::render_svg(dot_path, output_svg_path);
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(dot_path);
        return result;
    } catch (...) {
        m03gagbhsnusi43zogoacgj2ez_filesystem::remove(dot_path);
        throw ;
    }
}

} // namespace m03gagbhtahg11wzn32idilzte_module_graph
