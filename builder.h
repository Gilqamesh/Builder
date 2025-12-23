#ifndef BUILDER_PROJECT_BUILDER_BUILDER_H
# define BUILDER_PROJECT_BUILDER_BUILDER_H

# include "builder_plugin.h"
# include "compiler.h"

class builder_t {
public:
    static std::filesystem::path materialize_static_library(
        builder_ctx_t* ctx, const builder_api_t* api, const std::string& static_library_name,
        const std::vector<std::filesystem::path>& cpp_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values
    );
    static std::filesystem::path reference_static_library(builder_ctx_t* ctx, const builder_api_t* api, const std::filesystem::path& static_library_path);

    static std::filesystem::path materialize_shared_library(
        builder_ctx_t* ctx, const builder_api_t* api, const std::string& shared_library_name,
        const std::vector<std::filesystem::path>& cpp_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values
    );
    static std::filesystem::path reference_shared_library(builder_ctx_t* ctx, const builder_api_t* api, const std::filesystem::path& shared_library_path);

    static std::filesystem::path materialize_binary(
        builder_ctx_t* ctx, const builder_api_t* api, const std::string& binary_name,
        const std::vector<std::filesystem::path>& cpp_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        std::vector<compiler_t::binary_file_input_t> binary_inputs
    );
    static std::filesystem::path reference_binary(builder_ctx_t* ctx, const builder_api_t* api, const std::filesystem::path& binary_name);

private:
    static std::vector<std::filesystem::path> materialize_object_files(
        const std::filesystem::path& install_dir,
        const std::filesystem::path& modules_dir,
        const std::filesystem::path& source_dir,
        const std::vector<std::filesystem::path>& cpp_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        bool position_independent
    );
};

#endif // BUILDER_PROJECT_BUILDER_BUILDER_H
