#ifndef M03GN8RF3PEEW0RE4L1S2VVAW6_BOOTSTRAP_SEED_BOOTSTRAP_SEED_H
# define M03GN8RF3PEEW0RE4L1S2VVAW6_BOOTSTRAP_SEED_BOOTSTRAP_SEED_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

# include <vector>

namespace m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed {

/**
 * @brief Discovers the builder CLI module used as the active bootstrap seed.
 *
 * Materializes m03gagbhst621faiop1rztfkqp_builder_cli in graph when necessary.
 * The reference borrows a graph-owned module and requires graph to remain alive;
 * repeated discovery returns the same object. Discovery failures propagate.
 * This operation alone does not check ws0 placement; modules() validates placement
 * of the complete seed set. Serialize access when materializing objects in graph.
 */
m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& graph
);

/**
 * @brief Discovers the explicit set of modules compiled into the bootstrap seed.
 *
 * Materializes all seed members in graph and requires every member to exist in
 * ws0. Returns borrowed graph-owned pointers in seed-list order, not dependency
 * order. Keep graph alive while using them; changing the returned vector does not
 * change seed membership. Serial access to graph is required during discovery.
 *
 * Discovery failures propagate; a member outside ws0 throws std::runtime_error.
 * A failure can leave earlier members materialized. Membership and bootstrap
 * responsibility are described by the
 * [repository bootstrap model](../../docs/repository-model.md#bootstrap-and-module-execution).
 *
 * @code{.cpp}
 * #include <m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed/bootstrap_seed.h>
 * #include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
 * #include <cassert>
 *
 * namespace bootstrap_seed = m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed;
 * namespace workspace_graph = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;
 *
 * int main() {
 *     // The configured workspace contains the complete seed set in ws0.
 *     const auto invocation_context = workspace_graph::invocation_context();
 *     workspace_graph::workspace_graph_t graph(
 *         invocation_context.workspace_root, invocation_context.artifact_root
 *     );
 *     const auto modules = bootstrap_seed::modules(graph);
 *     const auto& module = bootstrap_seed::module(graph);
 *     assert(bootstrap_seed::is_module(module));
 *     const auto version = bootstrap_seed::version(graph);
 *     // Use borrowed modules before graph is destroyed; version is an owning value.
 * }
 * @endcode
 */
std::vector<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t*> modules(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& graph
);

/**
 * @brief Tests whether module has a seed-member identity and belongs to ws0.
 *
 * Does not discover other members or inspect source files, versions or artifacts.
 */
bool is_module(const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module);

/**
 * @brief Returns the maximum currently stored source version of the seed members.
 *
 * Calls modules(graph), with the same discovery, ws0-placement and failure behavior.
 * Uses each module's current version(); already materialized modules are not
 * refreshed from disk by this call. The returned value owns its version number.
 */
m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::version_t version(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& graph
);

/**
 * @brief Locates an existing builder plugin under the active seed's latest artifact.
 *
 * Calls module(graph) and checks its artifact_latest_dir()/builder/install/builder.so.
 * Returns an owning path without building, loading or checking plugin ABI, freshness,
 * file type or completion markers. Path existence at lookup time does not keep the
 * artifact alive or guarantee it will still exist when loaded.
 *
 * Throws std::runtime_error when the path is absent; the diagnostic directs callers
 * to run make bootstrap to recreate bootstrap artifacts. Discovery and filesystem
 * failures also propagate. Follow module()'s graph-access and lifetime requirements.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t builder_plugin_path(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& graph
);

} // namespace m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed

#endif // M03GN8RF3PEEW0RE4L1S2VVAW6_BOOTSTRAP_SEED_BOOTSTRAP_SEED_H
