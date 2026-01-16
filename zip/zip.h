#ifndef ZIP_ZIP_H
# define ZIP_ZIP_H

# include "../filesystem.h"

/**
 * zip
 *
 * Minimal ZIP archive support.
 *
 * Intended use:
 * - Packaging and extracting builder artifacts
 * - Deterministic, filesystem-backed operation
 *
 * Behavior:
 * - Operates on regular files only
 * - Stores paths relative to the input root
 * - Creates new archives; does not overwrite
 *
 * Limitations:
 * - Symlinks, special files, permissions, ownership are ignored
 * - Archive paths are trusted verbatim on extraction
 * - No path sanitization or security hardening
 *
 * Errors:
 * - All validation, I/O, and ZIP failures are reported via std::runtime_error
 */
namespace zip {

/**
 * Recursively archive regular files under `dir` into `install_zip_path` (.zip).
 */
filesystem::path_t zip(const filesystem::path_t& dir, const filesystem::path_t& install_zip_path);

/**
 * Extract all entries from `zip_path` (.zip) into `install_dir`.
*/
filesystem::path_t unzip(const filesystem::path_t& zip_path, const filesystem::path_t& install_dir);

} // namespace zip

#endif // ZIP_ZIP_H
