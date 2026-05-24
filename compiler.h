#ifndef KERNEL_CPP_BUILDER_COMPILER_H
# define KERNEL_CPP_BUILDER_COMPILER_H

# include "filesystem.h"

# include <vector>

namespace kernel {

namespace cpp_builder {

namespace compiler {

#ifndef KERNEL_CPP_BUILDER_CPP_COMPILER_PATH
# define KERNEL_CPP_BUILDER_CPP_COMPILER_PATH "/usr/bin/clang++"
#endif

#ifndef KERNEL_CPP_BUILDER_C_COMPILER_PATH
# define KERNEL_CPP_BUILDER_C_COMPILER_PATH "/usr/bin/clang"
#endif

#ifndef KERNEL_CPP_BUILDER_AR_PATH
# define KERNEL_CPP_BUILDER_AR_PATH "/usr/bin/ar"
#endif

inline constexpr const char* CPP_COMPILER_PATH = KERNEL_CPP_BUILDER_CPP_COMPILER_PATH;
inline constexpr const char* C_COMPILER_PATH = KERNEL_CPP_BUILDER_C_COMPILER_PATH;
inline constexpr const char* AR_PATH = KERNEL_CPP_BUILDER_AR_PATH;

filesystem::path_t create_static_library(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const filesystem::path_t& static_library
);

filesystem::path_t create_shared_library(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<filesystem::path_t>& dsos,
    const filesystem::path_t& shared_library
);

filesystem::path_t create_binary(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<std::vector<filesystem::path_t>>& library_groups,
    bool TEMP_assume_all_link_inputs_are_shared,
    const filesystem::path_t& binary
);

} // namespace compiler

} // namespace cpp_builder

} // namespace kernel

#endif // KERNEL_CPP_BUILDER_COMPILER_H
