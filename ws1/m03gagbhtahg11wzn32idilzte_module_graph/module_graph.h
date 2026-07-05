#ifndef M03GAGBHTAHG11WZN32IDILZTE_MODULE_GRAPH_MODULE_GRAPH_H
# define M03GAGBHTAHG11WZN32IDILZTE_MODULE_GRAPH_MODULE_GRAPH_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

namespace m03gagbhtahg11wzn32idilzte_module_graph {

/**
 * Writes a DOT graph for workspace_graph and returns output_dot_path.
 *
 * target_module is highlighted in the graph.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t write_dot(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& target_module,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_dot_path
);

/**
 * Writes an SVG graph for workspace_graph and returns output_svg_path.
 *
 * target_module is highlighted in the graph.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t render_svg(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& target_module,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_svg_path
);

} // namespace m03gagbhtahg11wzn32idilzte_module_graph

#endif // M03GAGBHTAHG11WZN32IDILZTE_MODULE_GRAPH_MODULE_GRAPH_H
