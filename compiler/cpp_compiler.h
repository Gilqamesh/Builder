#ifndef BUILDER_PROJECT_BUILDER_COMPILER_CPP_COMPILER_H
# define BUILDER_PROJECT_BUILDER_COMPILER_CPP_COMPILER_H

# include <modules/builder/module/module_graph.h>
# include <modules/builder/filesystem/filesystem.h>

# include <vector>

class cpp_compiler_t {
public:
    static path_t create_static_library(
        const builder_t* builder,
        const std::vector<path_t>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const relative_path_t& relative_static_library_path
    );

    static path_t create_shared_library(
        const builder_t* builder,
        const std::vector<path_t>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const relative_path_t& relative_shared_library_path
    );

    static path_t create_library(
        const builder_t* builder,
        const std::vector<path_t>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const relative_path_t& relative_libary_stem,
        library_type_t library_type
    );

    static path_t create_binary(
        const builder_t* builder,
        const std::vector<path_t>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        library_type_t library_type,
        const relative_path_t& relative_binary_path
    );
    
    static path_t reference_static_library(
        const builder_t* builder,
        const path_t& existing_static_library,
        const relative_path_t& relative_static_library_path
    );

    static path_t reference_shared_library(
        const builder_t* builder,
        const path_t& existing_shared_library,
        const relative_path_t& relative_shared_library_path
    );

    static path_t reference_library(
        const builder_t* builder,
        const path_t& existing_library,
        const relative_path_t& relative_library_path,
        library_type_t library_type
    );

    static path_t reference_binary(
        const builder_t* builder,
        const path_t& existing_binary,
        const relative_path_t& relative_binary_path
    );

public:
    static path_t create_static_library(
        const path_t& cache_dir,
        const path_t& source_dir,
        const std::vector<path_t>& include_dirs,
        const std::vector<path_t>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const path_t& static_library
    );

    static path_t create_shared_library(
        const path_t& cache_dir,
        const path_t& source_dir,
        const std::vector<path_t>& include_dirs,
        const std::vector<path_t>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::vector<path_t>& dsos,
        const path_t& shared_library
    );

    static path_t create_binary(
        const path_t& cache_dir,
        const path_t& source_dir,
        const std::vector<path_t>& include_dirs,
        const std::vector<path_t>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::vector<std::vector<path_t>>& library_groups,
        library_type_t library_type,
        const path_t& binary
    );

    static path_t reference_static_library(
        const path_t& existing_static_library,
        const path_t& static_library
    );

    static path_t reference_shared_library(
        const path_t& existing_shared_library,
        const path_t& shared_library
    );

    static path_t reference_shared_library(
        const path_t& existing_library,
        const path_t& library_stem,
        library_type_t library_type
    );

private:
    // TODO: rename to tmp
    static std::vector<path_t> cache_object_files(
        const builder_t* builder,
        const std::vector<path_t>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        library_type_t library_type
    );

    static std::vector<path_t> cache_object_files(
        const path_t& cache_dir,
        const path_t& source_dir,
        const std::vector<path_t>& include_dirs,
        const std::vector<path_t>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        bool position_independent
    );
};

#endif // BUILDER_PROJECT_BUILDER_COMPILER_CPP_COMPILER_H
