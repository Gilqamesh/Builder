#ifndef BUILDER_PROJECT_ZIP_ZIP_H
# define BUILDER_PROJECT_ZIP_ZIP_H

# include <modules/builder/filesystem/filesystem.h>

# include <vector>

class zip_t {
public:
    // Does not preserve non-regular files (symlinks, etc.)
    static path_t zip(const path_t& dir, const path_t& install_zip_path);
    static path_t zip(const std::vector<path_t>& regular_files, const path_t& install_zip_path);

    static path_t unzip(const path_t& zip_path, const path_t& install_dir);
};

#endif // BUILDER_PROJECT_ZIP_ZIP_H
