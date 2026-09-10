#ifndef M03GN8RF3PE86V64VPHNAAM6RL_SOURCE_DEPENDENCIES_SOURCE_DEPENDENCIES_H
# define M03GN8RF3PE86V64VPHNAAM6RL_SOURCE_DEPENDENCIES_SOURCE_DEPENDENCIES_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

# include <functional>
# include <vector>

namespace m03gn8rf3pe86v64vphnaam6rl_source_dependencies {

/**
 * @brief Selects the workspace dependency rule used while scanning includes.
 *
 * See [repository dependency rules](../../docs/repository-model.md#dependency-discovery).
 */
enum class dependency_mode_t {
    /// @brief Allows dependencies in the owner's workspace or an earlier workspace.
    MODULE,
    /// @brief Requires an earlier workspace, except between bootstrap seed members in ws0.
    BUILDER
};

/**
 * @brief Owns scanned local file paths and borrows the directly referenced modules.
 *
 * Mutating these vectors does not change the graph or files. Dependency pointers
 * refer to objects owned by the owner's workspace graph; keep that graph alive.
 */
struct scan_t {
    /// @brief Lists input files and recursively resolved local includes in first-visit order.
    ///
    /// Files are visited once per normalized absolute path, without resolving symlink aliases.
    std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t> local_files;
    /// @brief Lists unique direct dependencies sorted by workspace order and module name.
    std::vector<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t*> dependencies;
};

/**
 * @brief Returns source and header files that contribute to a module library.
 *
 * Recursively selects regular .c, .cpp, .h and .hpp files rooted at source_root.
 * Excludes every file named builder.cpp or cli.cpp and the top-level cli/ and test/
 * subtrees. The returned paths own their root/path values; no include scanning is
 * performed. Filesystem traversal failures propagate to the caller.
 */
std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t> library_source_files(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source_root
);

/**
 * @brief Scans source_files and local headers for direct module dependencies.
 *
 * Uses m03gn7qllwpi68ovctow4jrccj_lexer::include_paths(), including its lexical
 * limits: macros and conditional preprocessing are not evaluated. A recognized
 * module-name prefix indexed by module's graph creates an edge without checking
 * that the named header exists. The owner's own prefix creates no self-edge.
 * Absolute includes are ignored. Other includes are searched beside the including
 * file, then under the distinct source_files roots in input order; only existing
 * regular files are followed. Quoted and angle includes use the same lookup.
 * Unresolved local/external includes are skipped.
 *
 * Dependencies are materialized in module's graph and checked against dependency_mode.
 * Pass nullptr for excluded_module to retain all dependencies, or a pointer from
 * that graph to omit that module by pointer identity. Exclusion happens after
 * eligibility checks and does not cause the excluded module's headers to be scanned.
 * Dependency modules' source sets are not scanned by this operation.
 *
 * Empty source_files produces an empty scan. Throws std::runtime_error for an
 * ineligible dependency or an input/resolved local file that cannot be opened;
 * discovery failures also propagate. Local-include lookup failures are treated as
 * unresolved includes. Scanning can materialize modules even before a later failure;
 * it does not roll back the graph. Serialize access to the graph while scanning.
 */
scan_t scan_sources(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    const std::vector<m03gagbhsnusi43zogoacgj2ez_filesystem::rooted_path_t>& source_files,
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t* excluded_module,
    dependency_mode_t dependency_mode
);

/**
 * @brief Collects direct source dependencies and their transitive library dependencies.
 *
 * dependency_mode applies only to the initial scan of source_files. Subsequent
 * scans use MODULE eligibility and the library source sets supplied by
 * source_files_for_module, including when the initial mode is BUILDER.
 *
 * With include_owner, module is first and its callback-supplied library dependencies
 * are included in addition to the initial source_files dependencies. Otherwise the
 * owner is excluded throughout traversal, including cycles. Each module appears
 * once, in depth-first first-visit order, not topological/link order; cycles terminate.
 *
 * Provide a callable returning readable library source files for each visited
 * module. It is called synchronously once per included module (including the owner
 * only when include_owner is true), is not retained, and may return an empty set.
 * Keep captured state alive for this call and avoid concurrent graph mutation.
 * Callback and scan exceptions propagate; already materialized modules remain.
 *
 * Returned dependency pointers borrow graph-owned objects. The included owner
 * pointer borrows module itself. Keep both the graph and owner alive while using
 * the result; the returned vector transfers no module ownership.
 *
 * Example using library sources from an existing workspace with invocation roots
 * configured through m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::invocation_context():
 * @code{.cpp}
 * #include <m03gn8rf3pe86v64vphnaam6rl_source_dependencies/source_dependencies.h>
 * #include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
 *
 * namespace source_dependencies = m03gn8rf3pe86v64vphnaam6rl_source_dependencies;
 * namespace workspace_graph = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;
 *
 * int main() {
 *     const auto invocation_context = workspace_graph::invocation_context();
 *     workspace_graph::workspace_graph_t graph(
 *         invocation_context.workspace_root, invocation_context.artifact_root
 *     );
 *     const auto* module = graph.discover_module(workspace_graph::module_name_t(
 *         "m03gn7qllwpi68ovctow4jrccj_lexer"
 *     ));
 *     const auto source_files_for_module = [](const workspace_graph::module_t& module) {
 *         return source_dependencies::library_source_files(module.source_dir());
 *     };
 *     const auto source_files = source_files_for_module(*module);
 *     const auto modules = source_dependencies::dependency_modules(
 *         *module, source_files, true, source_dependencies::dependency_mode_t::MODULE,
 *         source_files_for_module
 *     );
 *     const auto names = source_dependencies::module_names(modules);
 *     // names owns its values; modules borrows objects from graph.
 * }
 * @endcode
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
 * @brief Copies module names while preserving input order and duplicates.
 *
 * Every pointer must be non-null and refer to a live module. The returned names
 * own their values and remain valid independently of the modules and graph.
 */
std::vector<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_name_t> module_names(
    const std::vector<m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t*>& modules
);

} // namespace m03gn8rf3pe86v64vphnaam6rl_source_dependencies

#endif // M03GN8RF3PE86V64VPHNAAM6RL_SOURCE_DEPENDENCIES_SOURCE_DEPENDENCIES_H
