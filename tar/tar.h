#ifndef BUILDER_PROJECT_TAR_TAR_H
# define BUILDER_PROJECT_TAR_TAR_H

# include <modules/builder/filesystem/filesystem.h>

class tar_t {
public:
    /**
     * Creates a `.tar` archive from a directory tree.
     *
     * - Archives all regular files under `dir` recursively.
     * - Paths inside the archive are stored relative to `dir`.
     * - File permissions, ownership, timestamps, and symlinks are not preserved.
     */
    static path_t tar(const path_t& dir, const path_t& install_tar_path);

    /**
     * Extracts a `.tar` archive into a directory.
     *
     * - Validates paths to prevent directory traversal.
     * - Creates parent directories as needed.
     * - Extracted files use default permissions (umask).
     */
    static path_t untar(const path_t& tar_path, const path_t& install_dir);
};

#endif // BUILDER_PROJECT_TAR_TAR_H
