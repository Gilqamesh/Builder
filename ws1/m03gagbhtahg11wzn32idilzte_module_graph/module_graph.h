#ifndef M03GAGBHTAHG11WZN32IDILZTE_MODULE_GRAPH_MODULE_GRAPH_H
# define M03GAGBHTAHG11WZN32IDILZTE_MODULE_GRAPH_MODULE_GRAPH_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

namespace m03gagbhtahg11wzn32idilzte_module_graph {

/**
 * @brief Writes a DOT graph showing each materialized module, its builder and their dependencies.
 *
 * Captures workspace_graph.modules() at entry and groups those modules into all
 * graph workspaces, including empty ones. Each module has a pale blue rounded box
 * and a pale orange builder ellipse. A "builds" edge runs from builder to its
 * module. A "module" edge runs from dependent module to dependency module; a
 * "builder" edge runs from the dependent's builder to dependency module. All
 * edges use solid dark styling; builder edges are distinguished by label and
 * source node, not dashes. Both nodes for target_module get heavier dark borders
 * when its address matches a captured module; an outside target highlights none.
 * This direction differs from the dependency IR DOT renderer's arrows.
 *
 * Scans each captured module through
 * m03gagbhsujjf63n0w3r2w4q6h_build_phases::discover_module_dependencies(), which
 * owns library-source/builder.cpp selection and delegates include eligibility.
 * Scanning can materialize more graph modules despite the const reference, but
 * edges to modules absent from the entry snapshot are omitted for this call.
 * Discover the desired modules and dependencies first to include those edges.
 * This operation does not compute a transitive closure.
 *
 * output_dot_path must end in .dot and not already exist; returns a copy of it.
 * Missing parents are created. Invalid extensions, existing output and detected
 * open/write failures throw std::runtime_error; filesystem, discovery and scan
 * errors propagate, including missing builder.cpp or ineligible dependencies.
 * Failure after opening can leave partial DOT output and newly materialized graph
 * modules; neither is rolled back. Remove partial output before retrying.
 * No Graphviz executable is required. The graph and target are borrowed only for
 * this call; keep both alive and serialize graph access while scanning. Coordinate
 * destination access too: the existence check is not an atomic reservation.
 *
 * For a configured workspace with readable sources and builder.cpp files,
 * discover all indexed modules before rendering. The target pointer borrows the
 * graph and remains valid while that graph stays alive at the same address.
 * @code{.cpp}
 * #include <m03gagbhtahg11wzn32idilzte_module_graph/module_graph.h>
 * #include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 *
 * namespace module_graph = m03gagbhtahg11wzn32idilzte_module_graph;
 * namespace graph = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;
 * namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
 *
 * int main() {
 *     const auto invocation_context = graph::invocation_context();
 *     graph::workspace_graph_t workspace_graph(
 *         invocation_context.workspace_root, invocation_context.artifact_root
 *     );
 *     for (const auto& module_name : workspace_graph.module_names()) {
 *         workspace_graph.discover_module(module_name);
 *     }
 *     const auto* target_module = workspace_graph.discover_module(graph::module_name_t(
 *         "m03gagbhtahg11wzn32idilzte_module_graph"
 *     ));
 *     // Both final destinations must be unused; reserve modules.svg_tmp.dot too.
 *     const auto dot_path = module_graph::write_dot(
 *         workspace_graph, *target_module, filesystem::path_t("graphs/modules.dot")
 *     );
 *     const auto svg_path = module_graph::render_svg(
 *         workspace_graph, *target_module, filesystem::path_t("graphs/modules.svg")
 *     );
 * }
 * @endcode
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t write_dot(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& target_module,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_dot_path
);

/**
 * @brief Renders the materialized module-and-builder graph as SVG through Graphviz.
 *
 * Uses write_dot() for graph membership, scanning, edge direction and target
 * highlighting; its borrowing and graph-access requirements also apply here.
 * Requires a new .svg destination and the Graphviz dot executable configured by
 * the owning builder of m03gagbht6ja46uikb1ltan0x8_dot::render_svg(). Missing
 * parents are created; returns a copy of output_svg_path. Invalid extensions,
 * existing SVG output, filesystem, scan and tool failures throw.
 *
 * Appends "_tmp.dot" to the entire SVG path: graphs/modules.svg uses
 * graphs/modules.svg_tmp.dot. Current implementation removes an existing entry
 * there before writing DOT, even if an existing SVG will later cause rejection.
 * Reserve both paths and serialize renders to that destination. The intermediate
 * is removed on success and cleanup is attempted on failure; failed Graphviz
 * output is also removed. Cleanup can throw, leave files behind or replace the
 * original error. Inspect remaining files before retrying; an existing SVG is
 * preserved. See write_dot() for a complete caller example.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t render_svg(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& workspace_graph,
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& target_module,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& output_svg_path
);

} // namespace m03gagbhtahg11wzn32idilzte_module_graph

#endif // M03GAGBHTAHG11WZN32IDILZTE_MODULE_GRAPH_MODULE_GRAPH_H
