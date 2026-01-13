#ifndef BUILDER_PROJECT_BUILDER_CMAKE_CMAKE_H
# define BUILDER_PROJECT_BUILDER_CMAKE_CMAKE_H

# include <builder/filesystem/filesystem.h>

# include <vector>
# include <optional>

class cmake_t {
public:
    static void configure(const path_t& source_dir, const path_t& build_dir, const std::vector<std::pair<std::string, std::string>>& define_key_values);

    static void build(const path_t& build_dir, std::optional<size_t> n_jobs);

    static void install(const path_t& install_dir);
};

#endif // BUILDER_PROJECT_BUILDER_CMAKE_CMAKE_H
