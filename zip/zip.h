#ifndef BUILDER_PROJECT_ZIP_ZIP_H
# define BUILDER_PROJECT_ZIP_ZIP_H

# include <filesystem>

class zip_t {
public:
    // Does not preserve non-regular files (symlinks, etc.)
    static std::filesystem::path zip(const std::filesystem::path& dir, const std::filesystem::path& install_zip_path);

    static std::filesystem::path unzip(const std::filesystem::path& zip_path, const std::filesystem::path& install_dir);
};

#endif // BUILDER_PROJECT_ZIP_ZIP_H
