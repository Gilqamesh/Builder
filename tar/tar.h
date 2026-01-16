#ifndef TAR_TAR_H
# define TAR_TAR_H

# include "../filesystem.h"

namespace tar {

/**
 * Creates a `.tar` archive from a directory tree.
 *
 * - Archives **regular files only** under `dir`, recursively.
 * - Directories, symlinks, hardlinks, and special files are ignored.
 * - Paths inside the archive are stored relative to `dir`.
 * - File permissions, ownership, timestamps, and link information are not preserved.
 */
filesystem::path_t tar(const filesystem::path_t& dir, const filesystem::path_t& install_tar_path);

/**
 * Extracts a `.tar` archive into a directory.
 *
 * - Extracts **regular file entries only**.
 * - Validates paths to prevent directory traversal.
 * - Creates parent directories as needed.
 * - Extracted files use default permissions (umask).
 * - Directory entries and metadata present in the archive are ignored.
 */
filesystem::path_t untar(const filesystem::path_t& tar_path, const filesystem::path_t& install_dir);

} // namespace tar

#endif // TAR_TAR_H
