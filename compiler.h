#ifndef COMPILER_H
# define COMPILER_H

# include "builder_ctx.h"
# include "builder_api.h"

# include <filesystem>
# include <vector>

class compiler_t {
public:
    static std::filesystem::path create_static_library(
        builder_ctx_t* ctx, const builder_api_t* api,
        const std::vector<std::string>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::string& static_library_name
    );

    static std::filesystem::path create_shared_library(
        builder_ctx_t* ctx, const builder_api_t* api,
        const std::vector<std::string>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::string& shared_library_name
    );

    static std::filesystem::path create_library(
        builder_ctx_t* ctx, const builder_api_t* api,
        const std::vector<std::string>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::string& library_stem,
        bundle_type_t bundle_type
    );

    static std::filesystem::path create_binary(
        builder_ctx_t* ctx, const builder_api_t* api,
        const std::vector<std::string>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        bundle_type_t bundle_type,
        const std::string& binary_name
    );

    static std::filesystem::path create_binary(
        builder_ctx_t* ctx, const builder_api_t* api,
        const std::vector<std::string>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::vector<std::vector<std::filesystem::path>>& library_groups,
        const std::string& binary_name
    );

    static std::filesystem::path reference_static_library(
        builder_ctx_t* ctx, const builder_api_t* api,
        const std::filesystem::path& existing_static_library,
        const std::string& static_library_name
    );

    static std::filesystem::path reference_shared_library(
        builder_ctx_t* ctx, const builder_api_t* api,
        const std::filesystem::path& existing_shared_library,
        const std::string& shared_library_name
    );

    static std::filesystem::path reference_library(
        builder_ctx_t* ctx, const builder_api_t* api,
        const std::filesystem::path& existing_library,
        const std::string& library_name,
        bundle_type_t bundle_type
    );

    static std::filesystem::path reference_binary(
        builder_ctx_t* ctx, const builder_api_t* api,
        const std::filesystem::path& existing_binary,
        const std::string& binary_name
    );

public:
    static std::filesystem::path create_static_library(
        const std::filesystem::path& cache_dir,
        const std::filesystem::path& source_dir,
        const std::vector<std::filesystem::path>& include_dirs,
        const std::vector<std::string>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::filesystem::path& static_library
    );

    static std::filesystem::path create_shared_library(
        const std::filesystem::path& cache_dir,
        const std::filesystem::path& source_dir,
        const std::vector<std::filesystem::path>& include_dirs,
        const std::vector<std::string>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::vector<std::filesystem::path>& dso_files,
        const std::filesystem::path& shared_library
    );

private:
    static std::vector<std::filesystem::path> cache_object_files(
        builder_ctx_t* ctx, const builder_api_t* api,
        const std::vector<std::string>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        bundle_type_t bundle_type
    );

    static std::vector<std::filesystem::path> cache_object_files(
        const std::filesystem::path& cache_dir,
        const std::filesystem::path& source_dir,
        const std::vector<std::filesystem::path>& include_dirs,
        const std::vector<std::string>& source_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        bool position_independent
    );
};

#endif // COMPILER_H
