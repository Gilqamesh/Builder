#ifndef BUILDER_PROJECT_BUILDER_TAR_TAR_H
# define BUILDER_PROJECT_BUILDER_TAR_TAR_H

# include "../filesystem/filesystem.h"


class tar_t {
public:
    /**
     * Creates a `.tar` archive from a directory tree.
     *
     * - Archives **regular files only** under `dir`, recursively.
     * - Directories, symlinks, hardlinks, and special files are ignored.
     * - Paths inside the archive are stored relative to `dir`.
     * - File permissions, ownership, timestamps, and link information are not preserved.
     */
    static path_t tar(const path_t& dir, const path_t& install_tar_path);

    /**
     * Extracts a `.tar` archive into a directory.
     *
     * - Extracts **regular file entries only**.
     * - Validates paths to prevent directory traversal.
     * - Creates parent directories as needed.
     * - Extracted files use default permissions (umask).
     * - Directory entries and metadata present in the archive are ignored.
     */
    static path_t untar(const path_t& tar_path, const path_t& install_dir);
};

#endif // BUILDER_PROJECT_BUILDER_TAR_TAR_H
