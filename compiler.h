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
    const std::vector<builder::output_t>& library_outputs,
    const filesystem::relative_path_t& relative_output_path
);

filesystem::path_t create_binary(
    const builder::binary_phase_t& phase,
    const std::vector<filesystem::relative_path_t>& relative_source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<builder::output_t>& library_outputs,
    bool TEMP_assume_all_link_inputs_are_shared,
    const filesystem::relative_path_t& relative_output_path
);

filesystem::path_t create_builder_shared_library(
    const filesystem::path_t& build_dir,
    const filesystem::path_t& source_dir,
    const std::vector<filesystem::path_t>& include_dirs,
    const filesystem::path_t& builder_source_file,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const std::vector<filesystem::path_t>& libraries,
    const filesystem::path_t& builder_plugin
);

} // namespace compiler

} // namespace cpp_builder

} // namespace kernel

#endif // KERNEL_CPP_BUILDER_COMPILER_H
