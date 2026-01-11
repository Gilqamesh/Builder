#ifndef BUILDER_PROJECT_TAR_TAR_H
# define BUILDER_PROJECT_TAR_TAR_H

# include <modules/builder/module/module_graph.h>

# include <filesystem>

class tar_t {
public:
    /**
     * Creates a `.tar` archive from a directory tree.
     *
     * - Archives all regular files under `dir` recursively.
     * - Paths inside the archive are stored relative to `dir`.
     * - File permissions, ownership, timestamps, and symlinks are not preserved.
     */
    static std::filesystem::path tar(const std::filesystem::path& dir, const std::filesystem::path& install_tar_path);

    /**
     * Extracts a `.tar` archive into a directory.
     *
     * - Validates paths to prevent directory traversal.
     * - Creates parent directories as needed.
     * - Extracted files use default permissions (umask).
     */
    static std::filesystem::path untar(const std::filesystem::path& tar_path, const std::filesystem::path& install_dir);
};

#endif // BUILDER_PROJECT_TAR_TAR_H
