#ifndef M03GN8RF3PEEW0RE4L1S2VVAW6_BOOTSTRAP_SEED_BOOTSTRAP_SEED_H
# define M03GN8RF3PEEW0RE4L1S2VVAW6_BOOTSTRAP_SEED_BOOTSTRAP_SEED_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

# include <vector>

namespace m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed {

/**
 * @brief Returns the module used as the active bootstrap seed.
 */
m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& graph
);

/**
 * @brief Returns all modules built into the bootstrap seed.
 */
std::vector<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t*> modules(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& graph
);

/**
 * @brief Returns true if module is part of the bootstrap seed.
 */
bool is_module(const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module);

/**
 * @brief Returns the newest source version across bootstrap seed modules.
 */
m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::version_t version(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& graph
);

/**
 * @brief Returns the bootstrap seed builder plugin path.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t builder_plugin_path(
    m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::workspace_graph_t& graph
);

} // namespace m03gn8rf3peew0re4l1s2vvaw6_bootstrap_seed

#endif // M03GN8RF3PEEW0RE4L1S2VVAW6_BOOTSTRAP_SEED_BOOTSTRAP_SEED_H
