#ifndef BUILDER_PROJECT_BUILDER_BUILDER_H
# define BUILDER_PROJECT_BUILDER_BUILDER_H

# include "builder_plugin.h"
# include "compiler.h"

class builder_t {
public:
    static std::filesystem::path lib(
        builder_ctx_t* ctx, const builder_api_t* api,
        const std::vector<std::string>& cpp_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values
    );

    static std::filesystem::path so(
        builder_ctx_t* ctx, const builder_api_t* api,
        const std::vector<std::string>& cpp_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values
    );

    static std::filesystem::path binary(
        builder_ctx_t* ctx, const builder_api_t* api,
        const std::vector<std::string>& cpp_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::string& bin_name,
        std::vector<compiler_t::binary_input_t> binary_inputs
    );
};

#endif // BUILDER_PROJECT_BUILDER_BUILDER_H
