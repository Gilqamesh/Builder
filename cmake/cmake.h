#ifndef CMAKE_CMAKE_H
# define CMAKE_CMAKE_H

# include "../filesystem/filesystem.h"

# include <vector>
# include <optional>

namespace cmake {

inline const constexpr char* CMAKE_PATH = "/usr/bin/cmake";

void configure(const filesystem::path_t& source_dir, const filesystem::path_t& build_dir, const std::vector<std::pair<std::string, std::string>>& define_key_values);

void build(const filesystem::path_t& build_dir, std::optional<size_t> n_jobs);

void install(const filesystem::path_t& install_dir);

} // namespace cmake

#endif // CMAKE_CMAKE_H
