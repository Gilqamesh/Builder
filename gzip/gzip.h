#ifndef BUILDER_PROJECT_BUILDER_GZIP_GZIP_H
# define BUILDER_PROJECT_BUILDER_GZIP_GZIP_H

# include <modules/builder/filesystem/filesystem.h>

class gzip_t {
public:
    /**
     * Compresses a regular file into a `.gz` file.
     *
     * - Input must be a regular file.
     * - Output contains the compressed byte stream of the input file.
     * - Does not preserve permissions, ownership, timestamps, or symlinks.
     */
    static path_t gzip(const path_t& dir, const path_t& install_gzip_path);

    /**
     * Decompresses a `.gz` file into a regular file.
     *
     * - Input must be a `.gz` file.
     * - Output is the decompressed byte stream.
     * - Caller determines interpretation of the output (e.g. `.tar`).
     */
    static path_t ungzip(const path_t& gzip_path, const path_t& install_file);
};

#endif // BUILDER_PROJECT_BUILDER_GZIP_GZIP_H
