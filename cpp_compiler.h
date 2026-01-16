#ifndef COMPILER_CPP_COMPILER_H
# define COMPILER_CPP_COMPILER_H

# include "filesystem.h"

# include <vector>

namespace cpp_compiler {

const constexpr char* CPP_COMPILER_PATH = "/usr/bin/clang++";
const constexpr char* C_COMPILER_PATH = "/usr/bin/clang";
const constexpr char* AR_PATH = "/usr/bin/ar";

filesystem::path_t create_static_library(
    const filesystem::path_t& cache_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const filesystem::path_t& static_library
);

filesystem::path_t create_shared_library(
    const filesystem::path_t& cache_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<filesystem::path_t>& dsos,
    const filesystem::path_t& shared_library
);

filesystem::path_t create_binary(
    const filesystem::path_t& cache_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<std::vector<filesystem::path_t>>& library_groups,
    bool TEMP_assume_all_link_inputs_are_shared,
    const filesystem::path_t& binary
);

} // namespace cpp_compiler

#endif // COMPILER_CPP_COMPILER_H
