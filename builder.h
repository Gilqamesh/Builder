#ifndef BUILDER_PROJECT_BUILDER_BUILDER_H
# define BUILDER_PROJECT_BUILDER_BUILDER_H

# include "builder_plugin.h"
# include "compiler.h"

class builder_t {
public:
    static std::filesystem::path lib(
        builder_ctx_t* ctx, const builder_api_t* api,
        const std::vector<std::filesystem::path>& cpp_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        bool position_independent
    );

    static std::filesystem::path so(
        builder_ctx_t* ctx, const builder_api_t* api,
        const std::vector<std::filesystem::path>& cpp_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values
    );

    static std::filesystem::path binary(
        builder_ctx_t* ctx, const builder_api_t* api,
        const std::vector<std::filesystem::path>& cpp_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::string& bin_name,
        std::vector<compiler_t::binary_file_input_t> binary_inputs,
        const std::vector<std::string>& additional_linker_flags
    );

private:
    static std::vector<std::filesystem::path> cpp_files_to_objects(
        const std::filesystem::path& artifact_dir,
        const std::filesystem::path& modules_dir,
        const std::filesystem::path& src_dir,
        const std::vector<std::filesystem::path>& cpp_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        bool position_independent
    );
};

#endif // BUILDER_PROJECT_BUILDER_BUILDER_H
