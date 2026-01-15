#ifndef BUILDER_CPP_COMPILER_H
# define BUILDER_CPP_COMPILER_H

# include "module_builder.h"
# include "cpp_compiler/cpp_compiler.h"

namespace builder::cpp_compiler {

filesystem::path_t create_static_library(
    const module_builder_t* module_builder,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const filesystem::relative_path_t& relative_static_library_path
);

filesystem::path_t create_shared_library(
    const module_builder_t* module_builder,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const filesystem::relative_path_t& relative_shared_library_path
);

filesystem::path_t create_library(
    const module_builder_t* module_builder,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    const filesystem::relative_path_t& relative_libary_stem,
    library_type_t library_type
);

filesystem::path_t create_binary(
    const module_builder_t* module_builder,
    const std::vector<filesystem::path_t>& source_files,
    const std::vector<std::pair<std::string, std::string>>& define_key_values,
    library_type_t library_type,
    const filesystem::relative_path_t& relative_binary_path
);

} // namespace builder::cpp_compiler

#endif // BUILDER_CPP_COMPILER_H
