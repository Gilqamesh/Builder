#ifndef KERNEL_CPP_BUILDER_COMPILER_H
# define KERNEL_CPP_BUILDER_COMPILER_H

# include "phase.h"

# include <string>
# include <utility>
# include <vector>

namespace kernel {

namespace cpp_builder {

namespace compiler {

#ifndef KERNEL_CPP_BUILDER_CXX_COMPILER_PATH
# define KERNEL_CPP_BUILDER_CXX_COMPILER_PATH "/usr/bin/clang++"
#endif

#ifndef KERNEL_CPP_BUILDER_CC_COMPILER_PATH
# define KERNEL_CPP_BUILDER_CC_COMPILER_PATH "/usr/bin/clang"
#endif

#ifndef KERNEL_CPP_BUILDER_AR_PATH
# define KERNEL_CPP_BUILDER_AR_PATH "/usr/bin/ar"
#endif

inline constexpr const char* CXX_COMPILER_PATH = KERNEL_CPP_BUILDER_CXX_COMPILER_PATH;
inline constexpr const char* CC_COMPILER_PATH = KERNEL_CPP_BUILDER_CC_COMPILER_PATH;
inline constexpr const char* AR_PATH = KERNEL_CPP_BUILDER_AR_PATH;

filesystem::path_t create_static_library(
    const builder::library_phase_t& phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const filesystem::relative_path_t& relative_output_path
);

filesystem::path_t create_shared_library(
    const builder::library_phase_t& phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<filesystem::path_t>& dsos,
    const filesystem::relative_path_t& relative_output_path
);

filesystem::path_t create_binary(
    const builder::binary_phase_t& phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<std::vector<filesystem::path_t>>& library_groups,
    bool TEMP_assume_all_link_inputs_are_shared,
    const filesystem::relative_path_t& relative_output_path
);

} // namespace compiler

} // namespace cpp_builder

} // namespace kernel

#endif // KERNEL_CPP_BUILDER_COMPILER_H
