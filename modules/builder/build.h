#ifndef MODULES_BUILDER_BUILD_H
# define MODULES_BUILDER_BUILD_H

# include <filesystem>
# include <string>
# include <vector>

# include "builder.h"
# include "compiler.h"

class build_t {
public:
    static std::filesystem::path lib(
        builder_ctx_t* ctx,
        const builder_api_t* api,
        const std::vector<std::string>& cpp_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values
    );

    static std::filesystem::path so(
        builder_ctx_t* ctx,
        const builder_api_t* api,
        const std::vector<std::string>& cpp_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values
    );

    static std::filesystem::path binary(
        builder_ctx_t* ctx,
        const builder_api_t* api,
        const std::vector<std::string>& cpp_files,
        const std::vector<std::pair<std::string, std::string>>& define_key_values,
        const std::string& bin_name,
        std::vector<compiler_t::binary_input_t> binary_inputs
    );
};

#endif // MODULES_BUILDER_BUILD_H
