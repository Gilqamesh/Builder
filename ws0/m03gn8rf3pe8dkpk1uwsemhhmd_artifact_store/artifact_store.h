#ifndef M03GN8RF3PE8DKPK1UWSEMHHMD_ARTIFACT_STORE_ARTIFACT_STORE_H
# define M03GN8RF3PE8DKPK1UWSEMHHMD_ARTIFACT_STORE_ARTIFACT_STORE_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

# include <optional>
# include <string_view>

namespace m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store {

/**
 * @brief Returns true if path exists or is a dangling symlink.
 *
 * Filesystem existence-query failures propagate; absence returns false.
 */
bool exists_or_symlink(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path);

/**
 * @brief Removes a file, symlink, or directory tree if it exists.
 *
 * A symlink itself is removed without removing its target. Missing paths are a
 * no-op. Filesystem failures propagate and directory removal may be partial.
 */
void remove_existing_path(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path);

/**
 * @brief Returns the in-progress marker path for artifact_dir.
 *
 * Selects artifact_dir/.started without creating or inspecting it.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t started_marker(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& artifact_dir);

/**
 * @brief Returns the complete marker path for artifact_dir.
 *
 * Selects artifact_dir/.complete without creating or inspecting it. The producer
 * creates this marker only after successful output construction and validation.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t completed_marker(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& artifact_dir);

/**
 * @brief Finds the lexically newest completed artifact with the requested cache-key suffix.
 *
 * Examines immediate child directories with a nonempty prefix followed by "-hash"
 * and an existing .complete marker. Lexical filename order selects the newest
 * canonical UTC-prefixed name; modification times and payloads are not checked.
 * Marker contents and .started are not inspected. Returns std::nullopt when root
 * is absent or no candidate matches; filesystem traversal failures propagate.
 */
std::optional<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> completed_artifact_dir_by_hash(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root,
    std::string_view hash
);

/**
 * @brief Selects a completed artifact for hash or a UTC-prefixed path for new work.
 *
 * Reuses completed_artifact_dir_by_hash() when possible; otherwise returns
 * root/YYYYMMDDTHHMMSS.nnnnnnnnnZ-hash. The hash is a caller-supplied cache key,
 * not computed here; use a single filename component so the artifact remains an
 * immediate child of root. No hash algorithm or hexadecimal width is required.
 *
 * Does not create, reserve, clean, validate or publish the selected directory.
 * A new selection is not a completed artifact or an exclusive reservation.
 * Callers coordinate concurrent producers, construct and validate output, manage
 * started/completed markers, and remove incomplete work on failure. Lookup and
 * path-construction exceptions propagate.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t versioned_artifact_dir(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root,
    std::string_view hash
);

/**
 * @brief Selects a versioned artifact beneath module.artifact_base_dir()/kind.
 *
 * Uses versioned_artifact_dir() and inherits its selection-only behavior. kind
 * names an output such as library or binary/cli; it does not select phase execution.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_dir(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& kind,
    std::string_view hash
);

/**
 * @brief Returns the latest symlink path for a module output kind.
 *
 * Selects module.artifact_latest_dir()/kind without accessing the filesystem.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t latest_dir(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& kind
);

/**
 * @brief Atomically updates latest_path to point at artifact_dir.
 *
 * Callers must establish successful completion before publishing; this operation
 * checks neither the target's existence nor its .complete marker. It returns
 * immediately if the existing symlink already names the same normalized path.
 *
 * Otherwise it removes any latest_path + "_tmp" entry, creates missing parents,
 * creates a temporary symlink and renames it over latest_path. An immediate parent
 * symlink is replaced by a real directory; an existing non-directory parent throws
 * std::runtime_error. The final symlink replacement is atomic, not the preparation.
 * Filesystem failures propagate and can leave the temporary link behind.
 * Serialize writers sharing a latest path or its parent setup, and reserve the
 * "_tmp" sibling for this operation. A retry removes a stale temporary entry.
 */
void update_latest_symlink(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& latest_path,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& artifact_dir
);

/**
 * @brief Atomically updates a module output kind's latest symlink.
 *
 * Calls update_latest_symlink(latest_dir(module, kind), artifact_dir), including
 * its completion precondition, temporary-path ownership and writer coordination.
 * Publication order is owned by the
 * [repository artifact rules](../../docs/repository-model.md#versions-and-artifacts).
 *
 * Example producer: copy a caller-selected input into a completed install tree.
 * module and its graph must remain alive during the call. The caller supplies a
 * cache key covering the input and serializes producers for this output.
 * @code{.cpp}
 * #include <m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store/artifact_store.h>
 * #include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
 * #include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>
 * #include <string_view>
 *
 * namespace artifact_store = m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store;
 * namespace filesystem = m03gagbhsnusi43zogoacgj2ez_filesystem;
 * namespace workspace_graph = m03gagbhsp2drqq3gkop8pzfrm_workspace_graph;
 *
 * void publish(const workspace_graph::module_t& module,
 *              const filesystem::path_t& input, std::string_view hash) {
 *     const filesystem::relative_path_t kind("source");
 *     const auto artifact_dir = artifact_store::artifact_dir(module, kind, hash);
 *     if (!filesystem::exists(artifact_store::completed_marker(artifact_dir))) {
 *         try {
 *             filesystem::create_directories(artifact_dir);
 *             filesystem::touch(artifact_store::started_marker(artifact_dir));
 *             const auto install_dir = artifact_dir / filesystem::relative_path_t("install");
 *             filesystem::create_directories(install_dir);
 *             filesystem::copy(input, install_dir / filesystem::relative_path_t("input.txt"));
 *             // This producer's output is complete when the checked copy succeeds.
 *             filesystem::touch(artifact_store::completed_marker(artifact_dir));
 *             filesystem::remove(artifact_store::started_marker(artifact_dir));
 *         } catch (...) {
 *             artifact_store::remove_existing_path(artifact_dir);
 *             throw;
 *         }
 *     }
 *     artifact_store::update_latest(module, kind, artifact_dir);
 *     // If publication throws, the completed output remains available for retry.
 * }
 * @endcode
 */
void update_latest(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& kind,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& artifact_dir
);

} // namespace m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store

#endif // M03GN8RF3PE8DKPK1UWSEMHHMD_ARTIFACT_STORE_ARTIFACT_STORE_H
