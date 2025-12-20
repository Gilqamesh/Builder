#include "builder.h"
#include "builder_plugin.h"
#include "builder_plugin_internal.h"
#include "compiler.h"

#include <string>
#include <filesystem>
#include <format>
#include <chrono>

BUILDER_EXTERN void builder__build_self(builder_ctx_t* ctx, const builder_api_t* api) {
    std::vector<std::string> lib_cpp_files;
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(api->src_dir(ctx)))) {
        const auto& path = entry.path();
        const auto filename = path.filename().string();
        if (path.extension() != ".cpp" || filename == BUILDER_DRIVER_CPP || filename == BUILDER_PLUGIN_CPP) {
            continue ;
        }
        lib_cpp_files.push_back(filename);
    }
    builder_t::lib(ctx, api, lib_cpp_files, {}, false);
    builder_t::lib(ctx, api, lib_cpp_files, {}, true);
    builder_t::so(ctx, api, lib_cpp_files, {});

    std::vector<std::filesystem::path> all_cpp_files;
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(api->src_dir(ctx)))) {
        const auto& path = entry.path();
        if (path.extension() != ".cpp") {
            continue ;
        }
        all_cpp_files.push_back(path);
    }
    builder_t::binary(ctx, api, all_cpp_files, { { "VERSION", std::to_string(VERSION) } }, BUILDER_DRIVER, { });
}

BUILDER_EXTERN void builder__build_module(builder_ctx_t* ctx, const builder_api_t* api, const char* static_libs) {
}
