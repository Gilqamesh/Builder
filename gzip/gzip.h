#ifndef GZIP_GZIP_H
# define GZIP_GZIP_H

# include "../filesystem/filesystem.h"

namespace gzip {

/**
 * Compresses a regular file into a `.gz` file.
 *
 * - Input must be a regular file.
 * - Output contains the compressed byte stream of the input file.
 * - Does not preserve permissions, ownership, timestamps, or symlinks.
 */
filesystem::path_t gzip(const filesystem::path_t& dir, const filesystem::path_t& install_gzip_path);

/**
 * Decompresses a `.gz` file into a regular file.
 *
 * - Input must be a `.gz` file.
 * - Output is the decompressed byte stream.
 * - Caller determines interpretation of the output (e.g. `.tar`).
 */
filesystem::path_t ungzip(const filesystem::path_t& gzip_path, const filesystem::path_t& install_file);

} // gzip

#endif // GZIP_GZIP_H
