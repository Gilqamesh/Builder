#ifndef M03GAGBHT3SVCX3IGN454LFUP3_CMAKE_CMAKE_H
# define M03GAGBHT3SVCX3IGN454LFUP3_CMAKE_CMAKE_H

# include <m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.h>

# include <cstddef>
# include <optional>
# include <string>
# include <utility>
# include <vector>

namespace cmake {

/**
 * Configures source_dir into build_dir with the given CMake -D values.
 */
void configure(
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& source_dir,
    const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir,
    const std::vector<std::pair<std::string, std::string>>& define_key_values
);

/**
 * Builds an already configured build_dir.
 *
 * n_jobs sets the parallel job count when provided.
 */
void build(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir, std::optional<std::size_t> n_jobs);

/**
 * Installs an already built build_dir.
 */
void install(const m03gagbhsnusi43zogoacgj2ez_filesystem::path_t& build_dir);

} // namespace cmake


#endif // M03GAGBHT3SVCX3IGN454LFUP3_CMAKE_CMAKE_H
