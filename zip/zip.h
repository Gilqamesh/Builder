#ifndef BUILDER_PROJECT_BUILDER_ZIP_ZIP_H
# define BUILDER_PROJECT_BUILDER_ZIP_ZIP_H

# include "../filesystem/filesystem.h"

/**
 * zip_t
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
class zip_t {
public:
    /**
     * Recursively archive regular files under `dir` into `install_zip_path` (.zip).
     */
    static path_t zip(const path_t& dir, const path_t& install_zip_path);

    /**
     * Extract all entries from `zip_path` (.zip) into `install_dir`.
    */
    static path_t unzip(const path_t& zip_path, const path_t& install_dir);
};

#endif // BUILDER_PROJECT_BUILDER_ZIP_ZIP_H
