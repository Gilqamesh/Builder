#ifndef BUILDER_PROJECT_COMPILER_CPP_COMPILER_H
# define BUILDER_PROJECT_COMPILER_CPP_COMPILER_H

# include <modules/builder/module/module_graph.h>

# include <filesystem>
# include <vector>

class cpp_compiler_t {
public:
    static std::filesystem::path create_static_library(
        const builder_t* builder,
        const std::vector<std::filesystem::path>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::string& static_library_name
    );

    static std::filesystem::path create_shared_library(
        const builder_t* builder,
        const std::vector<std::filesystem::path>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::string& shared_library_name
    );

    static std::filesystem::path create_library(
        const builder_t* builder,
        const std::vector<std::filesystem::path>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::string& library_stem,
        library_type_t library_type
    );

    static std::filesystem::path create_binary(
        const builder_t* builder,
        const std::vector<std::filesystem::path>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        library_type_t library_type,
        const std::string& binary_name
    );
    
    static std::filesystem::path reference_static_library(
        const builder_t* builder,
        const std::filesystem::path& existing_static_library,
        const std::string& static_library_name
    );

    static std::filesystem::path reference_shared_library(
        const builder_t* builder,
        const std::filesystem::path& existing_shared_library,
        const std::string& shared_library_name
    );

    static std::filesystem::path reference_library(
        const builder_t* builder,
        const std::filesystem::path& existing_library,
        const std::string& library_name,
        library_type_t library_type
    );

    static std::filesystem::path reference_binary(
        const builder_t* builder,
        const std::filesystem::path& existing_binary,
        const std::string& binary_name
    );

public:
    static std::filesystem::path create_static_library(
        const std::filesystem::path& cache_dir,
        const std::filesystem::path& source_dir,
        const std::vector<std::filesystem::path>& include_dirs,
        const std::vector<std::filesystem::path>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::filesystem::path& static_library
    );

    static std::filesystem::path create_shared_library(
        const std::filesystem::path& cache_dir,
        const std::filesystem::path& source_dir,
        const std::vector<std::filesystem::path>& include_dirs,
        const std::vector<std::filesystem::path>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::vector<std::filesystem::path>& dsos,
        const std::filesystem::path& shared_library
    );

    static std::filesystem::path create_binary(
        const std::filesystem::path& cache_dir,
        const std::filesystem::path& source_dir,
        const std::vector<std::filesystem::path>& include_dirs,
        const std::vector<std::filesystem::path>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::vector<std::vector<std::filesystem::path>>& library_groups,
        library_type_t library_type,
        const std::filesystem::path& binary
    );

    static std::filesystem::path reference_static_library(
        const std::filesystem::path& existing_static_library,
        const std::filesystem::path& static_library
    );

    static std::filesystem::path reference_shared_library(
        const std::filesystem::path& existing_shared_library,
        const std::filesystem::path& shared_library
    );

    static std::filesystem::path reference_shared_library(
        const std::filesystem::path& existing_library,
        const std::filesystem::path& library_stem,
        library_type_t library_type
    );

private:
    // TODO: rename to tmp
    static std::vector<std::filesystem::path> cache_object_files(
        const builder_t* builder,
        const std::vector<std::filesystem::path>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        library_type_t library_type
    );

    static std::vector<std::filesystem::path> cache_object_files(
        const std::filesystem::path& cache_dir,
        const std::filesystem::path& source_dir,
        const std::vector<std::filesystem::path>& include_dirs,
        const std::vector<std::filesystem::path>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        bool position_independent
    );
};

#endif // BUILDER_PROJECT_COMPILER_CPP_COMPILER_H
