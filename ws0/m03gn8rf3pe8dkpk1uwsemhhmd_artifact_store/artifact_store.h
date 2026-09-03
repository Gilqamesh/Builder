#ifndef M03GN8RF3PE8DKPK1UWSEMHHMD_ARTIFACT_STORE_ARTIFACT_STORE_H
# define M03GN8RF3PE8DKPK1UWSEMHHMD_ARTIFACT_STORE_ARTIFACT_STORE_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>
# include <m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.h>

# include <optional>
# include <string_view>

namespace m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store {

/**
 * @brief Returns true if path exists or is a dangling symlink.
 */
bool exists_or_symlink(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path);

/**
 * @brief Removes a file, symlink, or directory tree if it exists.
 */
void remove_existing_path(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& path);

/**
 * @brief Returns the in-progress marker path for artifact_dir.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t started_marker(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& artifact_dir);

/**
 * @brief Returns the complete marker path for artifact_dir.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t completed_marker(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& artifact_dir);

/**
 * @brief Returns the newest completed artifact under root whose name ends with hash.
 */
std::optional<m03gagbhsnusi43zogoacgj2ez_filesystem::path_t> completed_artifact_dir_by_hash(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root,
    std::string_view hash
);

/**
 * @brief Returns an existing completed artifact for hash or a new time-prefixed artifact path.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t versioned_artifact_dir(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& root,
    std::string_view hash
);

/**
 * @brief Returns the artifact directory for a module output kind and hash.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t artifact_dir(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& kind,
    std::string_view hash
);

/**
 * @brief Returns the latest symlink path for a module output kind.
 */
m03gagbhsnusi43zogoacgj2ez_filesystem::path_t latest_dir(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& kind
);

/**
 * @brief Atomically updates latest_path to point at artifact_dir.
 */
void update_latest_symlink(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& latest_path,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& artifact_dir
);

/**
 * @brief Atomically updates a module output kind's latest symlink.
 */
void update_latest(
    const m03gagbhsp2drqq3gkop8pzfrm_workspace_graph::module_t& module,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::relative_path_t& kind,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& artifact_dir
);

} // namespace m03gn8rf3pe8dkpk1uwsemhhmd_artifact_store

#endif // M03GN8RF3PE8DKPK1UWSEMHHMD_ARTIFACT_STORE_ARTIFACT_STORE_H
