#ifndef M03GN8RF3PE86V64VPHNAAM6RL_SOURCE_DEPENDENCIES_SOURCE_DEPENDENCIES_H
# define M03GN8RF3PE86V64VPHNAAM6RL_SOURCE_DEPENDENCIES_SOURCE_DEPENDENCIES_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

# include <functional>
# include <vector>

namespace m03gn8rf3pe86v64vphnaam6rl_source_dependencies {

/**
 * @brief Selects the workspace dependency rule used while scanning includes.
 */
enum class dependency_mode_t {
    MODULE,
    BUILDER
};

/**
 * @brief Direct scan result for a source set.
 */
struct scan_t {
    std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t> local_files;
    std::vector<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t*> dependencies;
};

/**
 * @brief Returns source and header files that contribute to a module library.
 */
std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t> library_source_files(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source_root
);

/**
 * @brief Scans source_files and local headers for direct module dependencies.
 */
scan_t scan_sources(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t>& source_files,
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t* excluded_module,
    dependency_mode_t dependency_mode
);

/**
 * @brief Returns the module dependency closure for source_files.
 */
std::vector<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t*> dependency_modules(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t>& source_files,
    bool include_owner,
    dependency_mode_t dependency_mode,
    const std::function<std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t>(
        const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module
    )>& source_files_for_module
);

/**
 * @brief Returns the names of modules in order.
 */
std::vector<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t> module_names(
    const std::vector<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t*>& modules
);

} // namespace m03gn8rf3pe86v64vphnaam6rl_source_dependencies

#endif // M03GN8RF3PE86V64VPHNAAM6RL_SOURCE_DEPENDENCIES_SOURCE_DEPENDENCIES_H
