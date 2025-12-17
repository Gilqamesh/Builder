#ifndef MODULES_BUILDER_BUILD_H
# define MODULES_BUILDER_BUILD_H

# include <filesystem>
# include <string>
# include <vector>

# include "builder.h"

class build_t {
public:
    static std::filesystem::path lib(builder_ctx_t* ctx, const builder_api_t* api, const std::vector<std::string>& cpp_files);
    static std::filesystem::path so(builder_ctx_t* ctx, const builder_api_t* api, const std::vector<std::string>& cpp_files);
    static std::filesystem::path binary(builder_ctx_t* ctx, const builder_api_t* api, const std::string& main_cpp_file, const std::string& bin_name, const char* static_libs);
};

#endif // MODULES_BUILDER_BUILD_H
